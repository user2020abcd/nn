/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "boxes/abstract_box.h"

namespace Ui {
class InputField;
class PasswordInput;
class LinkButton;
} // namespace Ui

class PasscodeBox : public BoxContent, public RPCSender {
	Q_OBJECT

public:
	PasscodeBox(QWidget*, bool turningOff);
	PasscodeBox(QWidget*, const QByteArray &newSalt, const QByteArray &curSalt, bool hasRecovery, const QString &hint, bool turningOff = false);

private slots:
	void onSave(bool force = false);
	void onBadOldPasscode();
	void onOldChanged();
	void onNewChanged();
	void onEmailChanged();
	void onRecoverByEmail();
	void onRecoverExpired();
	void onSubmit();

signals:
	void reloadPassword();

protected:
	void prepare() override;
	void setInnerFocus() override;

	void paintEvent(QPaintEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;

private:
	void closeReplacedBy();

	void setPasswordDone(const MTPBool &result);
	bool setPasswordFail(const RPCError &error);

	void recoverStarted(const MTPauth_PasswordRecovery &result);
	bool recoverStartFail(const RPCError &error);

	void recover();
	QString _pattern;

	QPointer<BoxContent> _replacedBy;
	bool _turningOff = false;
	bool _cloudPwd = false;
	mtpRequestId _setRequest = 0;

	QByteArray _newSalt, _curSalt;
	bool _hasRecovery = false;
	bool _skipEmailWarning = false;

	int _aboutHeight = 0;

	Text _about, _hintText;

	object_ptr<Ui::PasswordInput> _oldPasscode;
	object_ptr<Ui::PasswordInput> _newPasscode;
	object_ptr<Ui::PasswordInput> _reenterPasscode;
	object_ptr<Ui::InputField> _passwordHint;
	object_ptr<Ui::InputField> _recoverEmail;
	object_ptr<Ui::LinkButton> _recover;

	QString _oldError, _newError, _emailError;

};

class RecoverBox : public BoxContent, public RPCSender {
	Q_OBJECT

public:
	RecoverBox(QWidget*, const QString &pattern);

public slots:
	void onSubmit();
	void onCodeChanged();

signals:
	void reloadPassword();
	void recoveryExpired();

protected:
	void prepare() override;
	void setInnerFocus() override;

	void paintEvent(QPaintEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;

private:
	void codeSubmitDone(bool recover, const MTPauth_Authorization &result);
	bool codeSubmitFail(const RPCError &error);

	mtpRequestId _submitRequest = 0;

	QString _pattern;

	object_ptr<Ui::InputField> _recoverCode;

	QString _error;

};
