/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "core/crash_report_window.h"

#include "core/crash_reports.h"
#include "window/main_window.h"
#include "platform/platform_specific.h"
#include "application.h"
#include "base/zlib_help.h"
#include "autoupdater.h"

PreLaunchWindow *PreLaunchWindowInstance = nullptr;

PreLaunchWindow::PreLaunchWindow(QString title) {
	Fonts::Start();

	auto icon = Window::CreateIcon();
	setWindowIcon(icon);
	setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

	setWindowTitle(title.isEmpty() ? qsl("Telegram") : title);

	QPalette p(palette());
	p.setColor(QPalette::Background, QColor(255, 255, 255));
	setPalette(p);

	QLabel tmp(this);
	tmp.setText(qsl("Tmp"));
	_size = tmp.sizeHint().height();

	int paddingVertical = (_size / 2);
	int paddingHorizontal = _size;
	int borderRadius = (_size / 5);
	setStyleSheet(qsl("QPushButton { padding: %1px %2px; background-color: #ffffff; border-radius: %3px; }\nQPushButton#confirm:hover, QPushButton#cancel:hover { background-color: #e3f1fa; color: #2f9fea; }\nQPushButton#confirm { color: #2f9fea; }\nQPushButton#cancel { color: #aeaeae; }\nQLineEdit { border: 1px solid #e0e0e0; padding: 5px; }\nQLineEdit:focus { border: 2px solid #37a1de; padding: 4px; }").arg(paddingVertical).arg(paddingHorizontal).arg(borderRadius));
	if (!PreLaunchWindowInstance) {
		PreLaunchWindowInstance = this;
	}
}

void PreLaunchWindow::activate() {
	setWindowState(windowState() & ~Qt::WindowMinimized);
	setVisible(true);
	psActivateProcess();
	activateWindow();
}

PreLaunchWindow *PreLaunchWindow::instance() {
	return PreLaunchWindowInstance;
}

PreLaunchWindow::~PreLaunchWindow() {
	if (PreLaunchWindowInstance == this) {
		PreLaunchWindowInstance = nullptr;
	}
}

PreLaunchLabel::PreLaunchLabel(QWidget *parent) : QLabel(parent) {
	QFont labelFont(font());
	labelFont.setFamily(Fonts::GetOverride(qsl("Open Sans Semibold")));
	labelFont.setPixelSize(static_cast<PreLaunchWindow*>(parent)->basicSize());
	setFont(labelFont);

	QPalette p(palette());
	p.setColor(QPalette::Foreground, QColor(0, 0, 0));
	setPalette(p);
	show();
};

void PreLaunchLabel::setText(const QString &text) {
	QLabel::setText(text);
	updateGeometry();
	resize(sizeHint());
}

PreLaunchInput::PreLaunchInput(QWidget *parent, bool password) : QLineEdit(parent) {
	QFont logFont(font());
	logFont.setFamily(Fonts::GetOverride(qsl("Open Sans")));
	logFont.setPixelSize(static_cast<PreLaunchWindow*>(parent)->basicSize());
	setFont(logFont);

	QPalette p(palette());
	p.setColor(QPalette::Foreground, QColor(0, 0, 0));
	setPalette(p);

	QLineEdit::setTextMargins(0, 0, 0, 0);
	setContentsMargins(0, 0, 0, 0);
	if (password) {
		setEchoMode(QLineEdit::Password);
	}
	show();
};

PreLaunchLog::PreLaunchLog(QWidget *parent) : QTextEdit(parent) {
	QFont logFont(font());
	logFont.setFamily(Fonts::GetOverride(qsl("Open Sans")));
	logFont.setPixelSize(static_cast<PreLaunchWindow*>(parent)->basicSize());
	setFont(logFont);

	QPalette p(palette());
	p.setColor(QPalette::Foreground, QColor(96, 96, 96));
	setPalette(p);

	setReadOnly(true);
	setFrameStyle(QFrame::NoFrame | QFrame::Plain);
	viewport()->setAutoFillBackground(false);
	setContentsMargins(0, 0, 0, 0);
	document()->setDocumentMargin(0);
	show();
};

PreLaunchButton::PreLaunchButton(QWidget *parent, bool confirm) : QPushButton(parent) {
	setFlat(true);

	setObjectName(confirm ? "confirm" : "cancel");

	QFont closeFont(font());
	closeFont.setFamily(Fonts::GetOverride(qsl("Open Sans Semibold")));
	closeFont.setPixelSize(static_cast<PreLaunchWindow*>(parent)->basicSize());
	setFont(closeFont);

	setCursor(Qt::PointingHandCursor);
	show();
};

void PreLaunchButton::setText(const QString &text) {
	QPushButton::setText(text);
	updateGeometry();
	resize(sizeHint());
}

PreLaunchCheckbox::PreLaunchCheckbox(QWidget *parent) : QCheckBox(parent) {
	setTristate(false);
	setCheckState(Qt::Checked);

	QFont closeFont(font());
	closeFont.setFamily(Fonts::GetOverride(qsl("Open Sans Semibold")));
	closeFont.setPixelSize(static_cast<PreLaunchWindow*>(parent)->basicSize());
	setFont(closeFont);

	setCursor(Qt::PointingHandCursor);
	show();
};

void PreLaunchCheckbox::setText(const QString &text) {
	QCheckBox::setText(text);
	updateGeometry();
	resize(sizeHint());
}

NotStartedWindow::NotStartedWindow()
: _label(this)
, _log(this)
, _close(this) {
	_label.setText(qsl("Could not start Telegram Desktop!\nYou can see complete log below:"));

	_log.setPlainText(Logs::full());

	connect(&_close, SIGNAL(clicked()), this, SLOT(close()));
	_close.setText(qsl("CLOSE"));

	QRect scr(QApplication::primaryScreen()->availableGeometry());
	move(scr.x() + (scr.width() / 6), scr.y() + (scr.height() / 6));
	updateControls();
	show();
}

void NotStartedWindow::updateControls() {
	_label.show();
	_log.show();
	_close.show();

	QRect scr(QApplication::primaryScreen()->availableGeometry());
	QSize s(scr.width() / 2, scr.height() / 2);
	if (s == size()) {
		resizeEvent(0);
	} else {
		resize(s);
	}
}

void NotStartedWindow::closeEvent(QCloseEvent *e) {
	deleteLater();
}

void NotStartedWindow::resizeEvent(QResizeEvent *e) {
	int padding = _size;
	_label.setGeometry(padding, padding, width() - 2 * padding, _label.sizeHint().height());
	_log.setGeometry(padding, padding * 2 + _label.sizeHint().height(), width() - 2 * padding, height() - 4 * padding - _label.height() - _close.height());
	_close.setGeometry(width() - padding - _close.width(), height() - padding - _close.height(), _close.width(), _close.height());
}

LastCrashedWindow::LastCrashedWindow()
: _port(80)
, _label(this)
, _pleaseSendReport(this)
, _yourReportName(this)
, _minidump(this)
, _report(this)
, _send(this)
, _sendSkip(this, false)
, _networkSettings(this)
, _continue(this)
, _showReport(this)
, _saveReport(this)
, _getApp(this)
, _includeUsername(this)
, _reportText(QString::fromUtf8(Sandbox::LastCrashDump()))
, _reportShown(false)
, _reportSaved(false)
, _sendingState(Sandbox::LastCrashDump().isEmpty() ? SendingNoReport : SendingUpdateCheck)
, _updating(this)
, _sendingProgress(0)
, _sendingTotal(0)
, _checkReply(0)
, _sendReply(0)
#ifndef TDESKTOP_DISABLE_AUTOUPDATE
, _updatingCheck(this)
, _updatingSkip(this, false)
#endif // !TDESKTOP_DISABLE_AUTOUPDATE
{
	excludeReportUsername();

	if (!cAlphaVersion() && !cBetaVersion()) { // currently accept crash reports only from testers
		_sendingState = SendingNoReport;
	}
	if (_sendingState != SendingNoReport) {
		qint64 dumpsize = 0;
		QString dumpspath = cWorkingDir() + qsl("tdata/dumps");
#if defined Q_OS_MAC && !defined MAC_USE_BREAKPAD
		dumpspath += qsl("/completed");
#endif
		QString possibleDump = getReportField(qstr("minidump"), qstr("Minidump:"));
		if (!possibleDump.isEmpty()) {
			if (!possibleDump.startsWith('/')) {
				possibleDump = dumpspath + '/' + possibleDump;
			}
			if (!possibleDump.endsWith(qstr(".dmp"))) {
				possibleDump += qsl(".dmp");
			}
			QFileInfo possibleInfo(possibleDump);
			if (possibleInfo.exists()) {
				_minidumpName = possibleInfo.fileName();
				_minidumpFull = possibleInfo.absoluteFilePath();
				dumpsize = possibleInfo.size();
			}
		}
		if (_minidumpFull.isEmpty()) {
			QString maxDump, maxDumpFull;
            QDateTime maxDumpModified, workingModified = QFileInfo(cWorkingDir() + qsl("tdata/working")).lastModified();
			QFileInfoList list = QDir(dumpspath).entryInfoList();
            for (int32 i = 0, l = list.size(); i < l; ++i) {
                QString name = list.at(i).fileName();
                if (name.endsWith(qstr(".dmp"))) {
                    QDateTime modified = list.at(i).lastModified();
                    if (maxDump.isEmpty() || qAbs(workingModified.secsTo(modified)) < qAbs(workingModified.secsTo(maxDumpModified))) {
                        maxDump = name;
                        maxDumpModified = modified;
                        maxDumpFull = list.at(i).absoluteFilePath();
                        dumpsize = list.at(i).size();
                    }
                }
            }
            if (!maxDump.isEmpty() && qAbs(workingModified.secsTo(maxDumpModified)) < 10) {
                _minidumpName = maxDump;
                _minidumpFull = maxDumpFull;
            }
        }
		if (_minidumpName.isEmpty()) { // currently don't accept crash reports without dumps from google libraries
			_sendingState = SendingNoReport;
		} else {
			_minidump.setText(qsl("+ %1 (%2 KB)").arg(_minidumpName).arg(dumpsize / 1024));
		}
	}
	if (_sendingState != SendingNoReport) {
		QString version = getReportField(qstr("version"), qstr("Version:"));
		QString current = cBetaVersion() ? qsl("-%1").arg(cBetaVersion()) : QString::number(AppVersion);
		if (version != current) { // currently don't accept crash reports from not current app version
			_sendingState = SendingNoReport;
		}
	}

	_networkSettings.setText(qsl("NETWORK SETTINGS"));
	connect(&_networkSettings, SIGNAL(clicked()), this, SLOT(onNetworkSettings()));

	if (_sendingState == SendingNoReport) {
		_label.setText(qsl("Last time Telegram Desktop was not closed properly."));
	} else {
		_label.setText(qsl("Last time Telegram Desktop crashed :("));
	}

#ifndef TDESKTOP_DISABLE_AUTOUPDATE
	_updatingCheck.setText(qsl("TRY AGAIN"));
	connect(&_updatingCheck, SIGNAL(clicked()), this, SLOT(onUpdateRetry()));
	_updatingSkip.setText(qsl("SKIP"));
	connect(&_updatingSkip, SIGNAL(clicked()), this, SLOT(onUpdateSkip()));

	Sandbox::connect(SIGNAL(updateChecking()), this, SLOT(onUpdateChecking()));
	Sandbox::connect(SIGNAL(updateLatest()), this, SLOT(onUpdateLatest()));
	Sandbox::connect(SIGNAL(updateProgress(qint64,qint64)), this, SLOT(onUpdateDownloading(qint64,qint64)));
	Sandbox::connect(SIGNAL(updateFailed()), this, SLOT(onUpdateFailed()));
	Sandbox::connect(SIGNAL(updateReady()), this, SLOT(onUpdateReady()));

	switch (Sandbox::updatingState()) {
	case Application::UpdatingDownload:
		setUpdatingState(UpdatingDownload, true);
		setDownloadProgress(Sandbox::updatingReady(), Sandbox::updatingSize());
	break;
	case Application::UpdatingReady: setUpdatingState(UpdatingReady, true); break;
	default: setUpdatingState(UpdatingCheck, true); break;
	}

	cSetLastUpdateCheck(0);
	Sandbox::startUpdateCheck();
#else // !TDESKTOP_DISABLE_AUTOUPDATE
	_updating.setText(qsl("Please check if there is a new version available."));
	if (_sendingState != SendingNoReport) {
		_sendingState = SendingNone;
	}
#endif // else for !TDESKTOP_DISABLE_AUTOUPDATE

	_pleaseSendReport.setText(qsl("Please send us a crash report."));
	_yourReportName.setText(qsl("Your Report Tag: %1\nYour User Tag: %2").arg(QString(_minidumpName).replace(".dmp", "")).arg(Sandbox::UserTag(), 0, 16));
	_yourReportName.setCursor(style::cur_text);
	_yourReportName.setTextInteractionFlags(Qt::TextSelectableByMouse);

	_includeUsername.setText(qsl("Include username @%1 as your contact info").arg(_reportUsername));

	_report.setPlainText(_reportTextNoUsername);

	_showReport.setText(qsl("VIEW REPORT"));
	connect(&_showReport, SIGNAL(clicked()), this, SLOT(onViewReport()));
	_saveReport.setText(qsl("SAVE TO FILE"));
	connect(&_saveReport, SIGNAL(clicked()), this, SLOT(onSaveReport()));
	_getApp.setText(qsl("GET THE LATEST OFFICIAL VERSION OF TELEGRAM DESKTOP"));
	connect(&_getApp, SIGNAL(clicked()), this, SLOT(onGetApp()));

	_send.setText(qsl("SEND CRASH REPORT"));
	connect(&_send, SIGNAL(clicked()), this, SLOT(onSendReport()));

	_sendSkip.setText(qsl("SKIP"));
	connect(&_sendSkip, SIGNAL(clicked()), this, SLOT(onContinue()));
	_continue.setText(qsl("CONTINUE"));
	connect(&_continue, SIGNAL(clicked()), this, SLOT(onContinue()));

	QRect scr(QApplication::primaryScreen()->availableGeometry());
	move(scr.x() + (scr.width() / 6), scr.y() + (scr.height() / 6));
	updateControls();
	show();
}

void LastCrashedWindow::onViewReport() {
	_reportShown = !_reportShown;
	updateControls();
}

void LastCrashedWindow::onSaveReport() {
	QString to = QFileDialog::getSaveFileName(0, qsl("Telegram Crash Report"), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + qsl("/report.telegramcrash"), qsl("Telegram crash report (*.telegramcrash)"));
	if (!to.isEmpty()) {
		QFile file(to);
		if (file.open(QIODevice::WriteOnly)) {
			file.write(getCrashReportRaw());
			_reportSaved = true;
			updateControls();
		}
	}
}

QByteArray LastCrashedWindow::getCrashReportRaw() const {
	QByteArray result(Sandbox::LastCrashDump());
	if (!_reportUsername.isEmpty() && _includeUsername.checkState() != Qt::Checked) {
		result.replace((qsl("Username: ") + _reportUsername).toUtf8(), "Username: _not_included_");
	}
	return result;
}

void LastCrashedWindow::onGetApp() {
	QDesktopServices::openUrl(qsl("https://desktop.telegram.org"));
}

void LastCrashedWindow::excludeReportUsername() {
	QString prefix = qstr("Username:");
	QStringList lines = _reportText.split('\n');
	for (int32 i = 0, l = lines.size(); i < l; ++i) {
		if (lines.at(i).trimmed().startsWith(prefix)) {
			_reportUsername = lines.at(i).trimmed().mid(prefix.size()).trimmed();
			lines.removeAt(i);
			break;
		}
	}
	_reportTextNoUsername = _reportUsername.isEmpty() ? _reportText : lines.join('\n');
}

QString LastCrashedWindow::getReportField(const QLatin1String &name, const QLatin1String &prefix) {
	QStringList lines = _reportText.split('\n');
	for (int32 i = 0, l = lines.size(); i < l; ++i) {
		if (lines.at(i).trimmed().startsWith(prefix)) {
			QString data = lines.at(i).trimmed().mid(prefix.size()).trimmed();

			if (name == qstr("version")) {
				if (data.endsWith(qstr(" beta"))) {
					data = QString::number(-data.replace(QRegularExpression(qsl("[^\\d]")), "").toLongLong());
				} else {
					data = QString::number(data.replace(QRegularExpression(qsl("[^\\d]")), "").toLongLong());
				}
			}

			return data;
		}
	}
	return QString();
}

void LastCrashedWindow::addReportFieldPart(const QLatin1String &name, const QLatin1String &prefix, QHttpMultiPart *multipart) {
	QString data = getReportField(name, prefix);
	if (!data.isEmpty()) {
		QHttpPart reportPart;
		reportPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(qsl("form-data; name=\"%1\"").arg(name)));
		reportPart.setBody(data.toUtf8());
		multipart->append(reportPart);
	}
}

void LastCrashedWindow::onSendReport() {
	if (_checkReply) {
		_checkReply->deleteLater();
		_checkReply = nullptr;
	}
	if (_sendReply) {
		_sendReply->deleteLater();
		_sendReply = nullptr;
	}
	App::setProxySettings(_sendManager);

	QString apiid = getReportField(qstr("apiid"), qstr("ApiId:")), version = getReportField(qstr("version"), qstr("Version:"));
	_checkReply = _sendManager.get(QNetworkRequest(qsl("https://tdesktop.com/crash.php?act=query_report&apiid=%1&version=%2&dmp=%3&platform=%4").arg(apiid).arg(version).arg(minidumpFileName().isEmpty() ? 0 : 1).arg(cPlatformString())));

	connect(_checkReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onSendingError(QNetworkReply::NetworkError)));
	connect(_checkReply, SIGNAL(finished()), this, SLOT(onCheckingFinished()));

	_pleaseSendReport.setText(qsl("Sending crash report..."));
	_sendingState = SendingProgress;
	_reportShown = false;
	updateControls();
}

QString LastCrashedWindow::minidumpFileName() {
	QFileInfo dmpFile(_minidumpFull);
	if (dmpFile.exists() && dmpFile.size() > 0 && dmpFile.size() < 20 * 1024 * 1024 &&
		QRegularExpression(qsl("^[a-zA-Z0-9\\-]{1,64}\\.dmp$")).match(dmpFile.fileName()).hasMatch()) {
		return dmpFile.fileName();
	}
	return QString();
}

void LastCrashedWindow::onCheckingFinished() {
	if (!_checkReply || _sendReply) return;

	QByteArray result = _checkReply->readAll().trimmed();
	_checkReply->deleteLater();
	_checkReply = nullptr;

	LOG(("Crash report check for sending done, result: %1").arg(QString::fromUtf8(result)));

	if (result == "Old") {
		_pleaseSendReport.setText(qsl("This report is about some old version of Telegram Desktop."));
		_sendingState = SendingTooOld;
		updateControls();
		return;
	} else if (result == "Unofficial") {
		_pleaseSendReport.setText(qsl("You use some custom version of Telegram Desktop."));
		_sendingState = SendingUnofficial;
		updateControls();
		return;
	} else if (result != "Report") {
		_pleaseSendReport.setText(qsl("Thank you for your report!"));
		_sendingState = SendingDone;
		updateControls();

		CrashReports::Restart();
		return;
	}

	auto multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

	addReportFieldPart(qstr("platform"), qstr("Platform:"), multipart);
	addReportFieldPart(qstr("version"), qstr("Version:"), multipart);

	QHttpPart reportPart;
	reportPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
	reportPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"report\"; filename=\"report.telegramcrash\""));
	reportPart.setBody(getCrashReportRaw());
	multipart->append(reportPart);

	QString dmpName = minidumpFileName();
	if (!dmpName.isEmpty()) {
		QFile file(_minidumpFull);
		if (file.open(QIODevice::ReadOnly)) {
			QByteArray minidump = file.readAll();
			file.close();

			QString zipName = QString(dmpName).replace(qstr(".dmp"), qstr(".zip"));

			zlib::FileToWrite minidumpZip;

			zip_fileinfo zfi = { { 0, 0, 0, 0, 0, 0 }, 0, 0, 0 };
			QByteArray dmpNameUtf = dmpName.toUtf8();
			minidumpZip.openNewFile(dmpNameUtf.constData(), &zfi, nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
			minidumpZip.writeInFile(minidump.constData(), minidump.size());
			minidumpZip.closeFile();
			minidumpZip.close();

			if (minidumpZip.error() == ZIP_OK) {
				QHttpPart dumpPart;
				dumpPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
				dumpPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant(qsl("form-data; name=\"dump\"; filename=\"%1\"").arg(zipName)));
				dumpPart.setBody(minidumpZip.result());
				multipart->append(dumpPart);

				_minidump.setText(qsl("+ %1 (%2 KB)").arg(zipName).arg(minidumpZip.result().size() / 1024));
			}
		}
	}

	_sendReply = _sendManager.post(QNetworkRequest(qsl("https://tdesktop.com/crash.php?act=report")), multipart);
	multipart->setParent(_sendReply);

	connect(_sendReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onSendingError(QNetworkReply::NetworkError)));
	connect(_sendReply, SIGNAL(finished()), this, SLOT(onSendingFinished()));
	connect(_sendReply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(onSendingProgress(qint64,qint64)));

	updateControls();
}

void LastCrashedWindow::updateControls() {
	int padding = _size, h = padding + _networkSettings.height() + padding;

	_label.show();
#ifndef TDESKTOP_DISABLE_AUTOUPDATE
	h += _networkSettings.height() + padding;
	if (_updatingState == UpdatingFail && (_sendingState == SendingNoReport || _sendingState == SendingUpdateCheck)) {
		_networkSettings.show();
		_updatingCheck.show();
		_updatingSkip.show();
		_send.hide();
		_sendSkip.hide();
		_continue.hide();
		_pleaseSendReport.hide();
		_yourReportName.hide();
		_includeUsername.hide();
		_getApp.hide();
		_showReport.hide();
		_report.hide();
		_minidump.hide();
		_saveReport.hide();
		h += padding + _updatingCheck.height() + padding;
	} else {
		if (_updatingState == UpdatingCheck || _sendingState == SendingFail || _sendingState == SendingProgress) {
			_networkSettings.show();
		} else {
			_networkSettings.hide();
		}
		if (_updatingState == UpdatingNone || _updatingState == UpdatingLatest || _updatingState == UpdatingFail) {
			h += padding + _updatingCheck.height() + padding;
			if (_sendingState == SendingNoReport) {
				_pleaseSendReport.hide();
				_yourReportName.hide();
				_includeUsername.hide();
				_getApp.hide();
				_showReport.hide();
				_report.hide();
				_minidump.hide();
				_saveReport.hide();
				_send.hide();
				_sendSkip.hide();
				_continue.show();
			} else {
				h += _showReport.height() + padding + _yourReportName.height() + padding;
				_pleaseSendReport.show();
				_yourReportName.show();
				if (_reportUsername.isEmpty()) {
					_includeUsername.hide();
				} else {
					h += _includeUsername.height() + padding;
					_includeUsername.show();
				}
				if (_sendingState == SendingTooOld || _sendingState == SendingUnofficial) {
					QString verStr = getReportField(qstr("version"), qstr("Version:"));
					qint64 ver = verStr.isEmpty() ? 0 : verStr.toLongLong();
					if (!ver || (ver == AppVersion) || (ver < 0 && (-ver / 1000) == AppVersion)) {
						h += _getApp.height() + padding;
						_getApp.show();
						h -= _yourReportName.height() + padding; // hide report name
						_yourReportName.hide();
						if (!_reportUsername.isEmpty()) {
							h -= _includeUsername.height() + padding;
							_includeUsername.hide();
						}
					} else {
						_getApp.hide();
					}
					_showReport.hide();
					_report.hide();
					_minidump.hide();
					_saveReport.hide();
					_send.hide();
					_sendSkip.hide();
					_continue.show();
				} else {
					_getApp.hide();
					if (_reportShown) {
						h += (_pleaseSendReport.height() * 12.5) + padding + (_minidumpName.isEmpty() ? 0 : (_minidump.height() + padding));
						_report.show();
						if (_minidumpName.isEmpty()) {
							_minidump.hide();
						} else {
							_minidump.show();
						}
						if (_reportSaved || _sendingState == SendingFail || _sendingState == SendingProgress || _sendingState == SendingUploading) {
							_saveReport.hide();
						} else {
							_saveReport.show();
						}
						_showReport.hide();
					} else {
						_report.hide();
						_minidump.hide();
						_saveReport.hide();
						if (_sendingState == SendingFail || _sendingState == SendingProgress || _sendingState == SendingUploading) {
							_showReport.hide();
						} else {
							_showReport.show();
						}
					}
					if (_sendingState == SendingTooMany || _sendingState == SendingDone) {
						_send.hide();
						_sendSkip.hide();
						_continue.show();
					} else {
						if (_sendingState == SendingProgress || _sendingState == SendingUploading) {
							_send.hide();
						} else {
							_send.show();
						}
						_sendSkip.show();
						_continue.hide();
					}
				}
			}
		} else {
			_getApp.hide();
			_pleaseSendReport.hide();
			_yourReportName.hide();
			_includeUsername.hide();
			_showReport.hide();
			_report.hide();
			_minidump.hide();
			_saveReport.hide();
			_send.hide();
			_sendSkip.hide();
			_continue.hide();
		}
		_updatingCheck.hide();
		if (_updatingState == UpdatingCheck || _updatingState == UpdatingDownload) {
			h += padding + _updatingSkip.height() + padding;
			_updatingSkip.show();
		} else {
			_updatingSkip.hide();
		}
	}
#else // !TDESKTOP_DISABLE_AUTOUPDATE
	h += _networkSettings.height() + padding;
	h += padding + _send.height() + padding;
	if (_sendingState == SendingNoReport) {
		_pleaseSendReport.hide();
		_yourReportName.hide();
		_includeUsername.hide();
		_showReport.hide();
		_report.hide();
		_minidump.hide();
		_saveReport.hide();
		_send.hide();
		_sendSkip.hide();
		_continue.show();
		_networkSettings.hide();
	} else {
		h += _showReport.height() + padding + _yourReportName.height() + padding;
		_pleaseSendReport.show();
		_yourReportName.show();
		if (_reportUsername.isEmpty()) {
			_includeUsername.hide();
		} else {
			h += _includeUsername.height() + padding;
			_includeUsername.show();
		}
		if (_reportShown) {
			h += (_pleaseSendReport.height() * 12.5) + padding + (_minidumpName.isEmpty() ? 0 : (_minidump.height() + padding));
			_report.show();
			if (_minidumpName.isEmpty()) {
				_minidump.hide();
			} else {
				_minidump.show();
			}
			_showReport.hide();
			if (_reportSaved || _sendingState == SendingFail || _sendingState == SendingProgress || _sendingState == SendingUploading) {
				_saveReport.hide();
			} else {
				_saveReport.show();
			}
		} else {
			_report.hide();
			_minidump.hide();
			_saveReport.hide();
			if (_sendingState == SendingFail || _sendingState == SendingProgress || _sendingState == SendingUploading) {
				_showReport.hide();
			} else {
				_showReport.show();
			}
		}
		if (_sendingState == SendingDone) {
			_send.hide();
			_sendSkip.hide();
			_continue.show();
			_networkSettings.hide();
		} else {
			if (_sendingState == SendingProgress || _sendingState == SendingUploading) {
				_send.hide();
			} else {
				_send.show();
			}
			_sendSkip.show();
			if (_sendingState == SendingFail) {
				_networkSettings.show();
			} else {
				_networkSettings.hide();
			}
			_continue.hide();
		}
	}

	_getApp.show();
	h += _networkSettings.height() + padding;
#endif // else for !TDESKTOP_DISABLE_AUTOUPDATE

	QRect scr(QApplication::primaryScreen()->availableGeometry());
	QSize s(2 * padding + QFontMetrics(_label.font()).width(qsl("Last time Telegram Desktop was not closed properly.")) + padding + _networkSettings.width(), h);
	if (s == size()) {
		resizeEvent(0);
	} else {
		resize(s);
	}
}

void LastCrashedWindow::onNetworkSettings() {
	auto &p = Sandbox::PreLaunchProxy();
	NetworkSettingsWindow *box = new NetworkSettingsWindow(this, p.host, p.port ? p.port : 80, p.user, p.password);
	connect(box, SIGNAL(saved(QString, quint32, QString, QString)), this, SLOT(onNetworkSettingsSaved(QString, quint32, QString, QString)));
	box->show();
}

void LastCrashedWindow::onNetworkSettingsSaved(QString host, quint32 port, QString username, QString password) {
	Sandbox::RefPreLaunchProxy().host = host;
	Sandbox::RefPreLaunchProxy().port = port ? port : 80;
	Sandbox::RefPreLaunchProxy().user = username;
	Sandbox::RefPreLaunchProxy().password = password;
#ifndef TDESKTOP_DISABLE_AUTOUPDATE
	if ((_updatingState == UpdatingFail && (_sendingState == SendingNoReport || _sendingState == SendingUpdateCheck)) || (_updatingState == UpdatingCheck)) {
		Sandbox::stopUpdate();
		cSetLastUpdateCheck(0);
		Sandbox::startUpdateCheck();
	} else
#endif // !TDESKTOP_DISABLE_AUTOUPDATE
	if (_sendingState == SendingFail || _sendingState == SendingProgress) {
		onSendReport();
	}
	activate();
}

#ifndef TDESKTOP_DISABLE_AUTOUPDATE
void LastCrashedWindow::setUpdatingState(UpdatingState state, bool force) {
	if (_updatingState != state || force) {
		_updatingState = state;
		switch (state) {
		case UpdatingLatest:
			_updating.setText(qsl("Latest version is installed."));
			if (_sendingState == SendingNoReport) {
				QTimer::singleShot(0, this, SLOT(onContinue()));
			} else {
				_sendingState = SendingNone;
			}
		break;
		case UpdatingReady:
			if (checkReadyUpdate()) {
				cSetRestartingUpdate(true);
				App::quit();
				return;
			} else {
				setUpdatingState(UpdatingFail);
				return;
			}
		break;
		case UpdatingCheck:
			_updating.setText(qsl("Checking for updates..."));
		break;
		case UpdatingFail:
			_updating.setText(qsl("Update check failed :("));
		break;
		}
		updateControls();
	}
}

void LastCrashedWindow::setDownloadProgress(qint64 ready, qint64 total) {
	qint64 readyTenthMb = (ready * 10 / (1024 * 1024)), totalTenthMb = (total * 10 / (1024 * 1024));
	QString readyStr = QString::number(readyTenthMb / 10) + '.' + QString::number(readyTenthMb % 10);
	QString totalStr = QString::number(totalTenthMb / 10) + '.' + QString::number(totalTenthMb % 10);
	QString res = qsl("Downloading update {ready} / {total} MB..").replace(qstr("{ready}"), readyStr).replace(qstr("{total}"), totalStr);
	if (_newVersionDownload != res) {
		_newVersionDownload = res;
		_updating.setText(_newVersionDownload);
		updateControls();
	}
}

void LastCrashedWindow::onUpdateRetry() {
	cSetLastUpdateCheck(0);
	Sandbox::startUpdateCheck();
}

void LastCrashedWindow::onUpdateSkip() {
	if (_sendingState == SendingNoReport) {
		onContinue();
	} else {
		if (_updatingState == UpdatingCheck || _updatingState == UpdatingDownload) {
			Sandbox::stopUpdate();
			setUpdatingState(UpdatingFail);
		}
		_sendingState = SendingNone;
		updateControls();
	}
}

void LastCrashedWindow::onUpdateChecking() {
	setUpdatingState(UpdatingCheck);
}

void LastCrashedWindow::onUpdateLatest() {
	setUpdatingState(UpdatingLatest);
}

void LastCrashedWindow::onUpdateDownloading(qint64 ready, qint64 total) {
	setUpdatingState(UpdatingDownload);
	setDownloadProgress(ready, total);
}

void LastCrashedWindow::onUpdateReady() {
	setUpdatingState(UpdatingReady);
}

void LastCrashedWindow::onUpdateFailed() {
	setUpdatingState(UpdatingFail);
}
#endif // !TDESKTOP_DISABLE_AUTOUPDATE

void LastCrashedWindow::onContinue() {
	if (CrashReports::Restart() == CrashReports::CantOpen) {
		new NotStartedWindow();
	} else if (!Global::started()) {
		Sandbox::launch();
	}
	close();
}

void LastCrashedWindow::onSendingError(QNetworkReply::NetworkError e) {
	LOG(("Crash report sending error: %1").arg(e));

	_pleaseSendReport.setText(qsl("Sending crash report failed :("));
	_sendingState = SendingFail;
	if (_checkReply) {
		_checkReply->deleteLater();
		_checkReply = nullptr;
	}
	if (_sendReply) {
		_sendReply->deleteLater();
		_sendReply = nullptr;
	}
	updateControls();
}

void LastCrashedWindow::onSendingFinished() {
	if (_sendReply) {
		QByteArray result = _sendReply->readAll();
		LOG(("Crash report sending done, result: %1").arg(QString::fromUtf8(result)));

		_sendReply->deleteLater();
		_sendReply = nullptr;
		_pleaseSendReport.setText(qsl("Thank you for your report!"));
		_sendingState = SendingDone;
		updateControls();

		CrashReports::Restart();
	}
}

void LastCrashedWindow::onSendingProgress(qint64 uploaded, qint64 total) {
	if (_sendingState != SendingProgress && _sendingState != SendingUploading) return;
	_sendingState = SendingUploading;

	if (total < 0) {
		_pleaseSendReport.setText(qsl("Sending crash report %1 KB...").arg(uploaded / 1024));
	} else {
		_pleaseSendReport.setText(qsl("Sending crash report %1 / %2 KB...").arg(uploaded / 1024).arg(total / 1024));
	}
	updateControls();
}

void LastCrashedWindow::closeEvent(QCloseEvent *e) {
	deleteLater();
}

void LastCrashedWindow::resizeEvent(QResizeEvent *e) {
	int padding = _size;
	_label.move(padding, padding + (_networkSettings.height() - _label.height()) / 2);

	_send.move(width() - padding - _send.width(), height() - padding - _send.height());
	if (_sendingState == SendingProgress || _sendingState == SendingUploading) {
		_sendSkip.move(width() - padding - _sendSkip.width(), height() - padding - _sendSkip.height());
	} else {
		_sendSkip.move(width() - padding - _send.width() - padding - _sendSkip.width(), height() - padding - _sendSkip.height());
	}

	_updating.move(padding, padding * 2 + _networkSettings.height() + (_networkSettings.height() - _updating.height()) / 2);

#ifndef TDESKTOP_DISABLE_AUTOUPDATE
	_pleaseSendReport.move(padding, padding * 2 + _networkSettings.height() + _networkSettings.height() + padding + (_showReport.height() - _pleaseSendReport.height()) / 2);
	_showReport.move(padding * 2 + _pleaseSendReport.width(), padding * 2 + _networkSettings.height() + _networkSettings.height() + padding);
	_yourReportName.move(padding, _showReport.y() + _showReport.height() + padding);
	_includeUsername.move(padding, _yourReportName.y() + _yourReportName.height() + padding);
	_getApp.move((width() - _getApp.width()) / 2, _showReport.y() + _showReport.height() + padding);

	if (_sendingState == SendingFail || _sendingState == SendingProgress) {
		_networkSettings.move(padding * 2 + _pleaseSendReport.width(), padding * 2 + _networkSettings.height() + _networkSettings.height() + padding);
	} else {
		_networkSettings.move(padding * 2 + _updating.width(), padding * 2 + _networkSettings.height());
	}

	if (_updatingState == UpdatingCheck || _updatingState == UpdatingDownload) {
		_updatingCheck.move(width() - padding - _updatingCheck.width(), height() - padding - _updatingCheck.height());
		_updatingSkip.move(width() - padding - _updatingSkip.width(), height() - padding - _updatingSkip.height());
	} else {
		_updatingCheck.move(width() - padding - _updatingCheck.width(), height() - padding - _updatingCheck.height());
		_updatingSkip.move(width() - padding - _updatingCheck.width() - padding - _updatingSkip.width(), height() - padding - _updatingSkip.height());
	}
#else // !TDESKTOP_DISABLE_AUTOUPDATE
	_getApp.move((width() - _getApp.width()) / 2, _updating.y() + _updating.height() + padding);

	_pleaseSendReport.move(padding, padding * 2 + _networkSettings.height() + _networkSettings.height() + padding + _getApp.height() + padding + (_showReport.height() - _pleaseSendReport.height()) / 2);
	_showReport.move(padding * 2 + _pleaseSendReport.width(), padding * 2 + _networkSettings.height() + _networkSettings.height() + padding + _getApp.height() + padding);
	_yourReportName.move(padding, _showReport.y() + _showReport.height() + padding);
	_includeUsername.move(padding, _yourReportName.y() + _yourReportName.height() + padding);

	_networkSettings.move(padding * 2 + _pleaseSendReport.width(), padding * 2 + _networkSettings.height() + _networkSettings.height() + padding + _getApp.height() + padding);
#endif // else for !TDESKTOP_DISABLE_AUTOUPDATE
	if (_reportUsername.isEmpty()) {
		_report.setGeometry(padding, _yourReportName.y() + _yourReportName.height() + padding, width() - 2 * padding, _pleaseSendReport.height() * 12.5);
	} else {
		_report.setGeometry(padding, _includeUsername.y() + _includeUsername.height() + padding, width() - 2 * padding, _pleaseSendReport.height() * 12.5);
	}
	_minidump.move(padding, _report.y() + _report.height() + padding);
	_saveReport.move(_showReport.x(), _showReport.y());

	_continue.move(width() - padding - _continue.width(), height() - padding - _continue.height());
}

NetworkSettingsWindow::NetworkSettingsWindow(QWidget *parent, QString host, quint32 port, QString username, QString password)
: PreLaunchWindow(qsl("HTTP Proxy Settings"))
, _hostLabel(this)
, _portLabel(this)
, _usernameLabel(this)
, _passwordLabel(this)
, _hostInput(this)
, _portInput(this)
, _usernameInput(this)
, _passwordInput(this, true)
, _save(this)
, _cancel(this, false)
, _parent(parent) {
	setWindowModality(Qt::ApplicationModal);

	_hostLabel.setText(qsl("Hostname"));
	_portLabel.setText(qsl("Port"));
	_usernameLabel.setText(qsl("Username"));
	_passwordLabel.setText(qsl("Password"));

	_save.setText(qsl("SAVE"));
	connect(&_save, SIGNAL(clicked()), this, SLOT(onSave()));
	_cancel.setText(qsl("CANCEL"));
	connect(&_cancel, SIGNAL(clicked()), this, SLOT(close()));

	_hostInput.setText(host);
	_portInput.setText(QString::number(port));
	_usernameInput.setText(username);
	_passwordInput.setText(password);

	QRect scr(QApplication::primaryScreen()->availableGeometry());
	move(scr.x() + (scr.width() / 6), scr.y() + (scr.height() / 6));
	updateControls();
	show();

	_hostInput.setFocus();
	_hostInput.setCursorPosition(_hostInput.text().size());
}

void NetworkSettingsWindow::resizeEvent(QResizeEvent *e) {
	int padding = _size;
	_hostLabel.move(padding, padding);
	_hostInput.setGeometry(_hostLabel.x(), _hostLabel.y() + _hostLabel.height(), 2 * _hostLabel.width(), _hostInput.height());
	_portLabel.move(padding + _hostInput.width() + padding, padding);
	_portInput.setGeometry(_portLabel.x(), _portLabel.y() + _portLabel.height(), width() - padding - _portLabel.x(), _portInput.height());
	_usernameLabel.move(padding, _hostInput.y() + _hostInput.height() + padding);
	_usernameInput.setGeometry(_usernameLabel.x(), _usernameLabel.y() + _usernameLabel.height(), (width() - 3 * padding) / 2, _usernameInput.height());
	_passwordLabel.move(padding + _usernameInput.width() + padding, _usernameLabel.y());
	_passwordInput.setGeometry(_passwordLabel.x(), _passwordLabel.y() + _passwordLabel.height(), width() - padding - _passwordLabel.x(), _passwordInput.height());

	_save.move(width() - padding - _save.width(), height() - padding - _save.height());
	_cancel.move(_save.x() - padding - _cancel.width(), _save.y());
}

void NetworkSettingsWindow::onSave() {
	QString host = _hostInput.text().trimmed(), port = _portInput.text().trimmed(), username = _usernameInput.text().trimmed(), password = _passwordInput.text().trimmed();
	if (!port.isEmpty() && !port.toUInt()) {
		_portInput.setFocus();
		return;
	} else if (!host.isEmpty() && port.isEmpty()) {
		_portInput.setFocus();
		return;
	}
	emit saved(host, port.toUInt(), username, password);
	close();
}

void NetworkSettingsWindow::closeEvent(QCloseEvent *e) {
}

void NetworkSettingsWindow::updateControls() {
	_hostInput.updateGeometry();
	_hostInput.resize(_hostInput.sizeHint());
	_portInput.updateGeometry();
	_portInput.resize(_portInput.sizeHint());
	_usernameInput.updateGeometry();
	_usernameInput.resize(_usernameInput.sizeHint());
	_passwordInput.updateGeometry();
	_passwordInput.resize(_passwordInput.sizeHint());

	int padding = _size;
	int w = 2 * padding + _hostLabel.width() * 2 + padding + _portLabel.width() * 2 + padding;
	int h = padding + _hostLabel.height() + _hostInput.height() + padding + _usernameLabel.height() + _usernameInput.height() + padding + _save.height() + padding;
	if (w == width() && h == height()) {
		resizeEvent(0);
	} else {
		setGeometry(_parent->x() + (_parent->width() - w) / 2, _parent->y() + (_parent->height() - h) / 2, w, h);
	}
}
