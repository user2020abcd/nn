/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "history/history_top_bar_widget.h"

#include <rpl/combine.h>
#include <rpl/combine_previous.h>
#include "styles/style_window.h"
#include "styles/style_dialogs.h"
#include "styles/style_history.h"
#include "styles/style_info.h"
#include "boxes/add_contact_box.h"
#include "boxes/confirm_box.h"
#include "info/info_memento.h"
#include "info/info_controller.h"
#include "storage/storage_shared_media.h"
#include "mainwidget.h"
#include "mainwindow.h"
#include "shortcuts.h"
#include "auth_session.h"
#include "lang/lang_keys.h"
#include "ui/special_buttons.h"
#include "ui/unread_badge.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/dropdown_menu.h"
#include "window/window_controller.h"
#include "window/window_peer_menu.h"
#include "calls/calls_instance.h"
#include "data/data_peer_values.h"
#include "observer_peer.h"
#include "apiwrap.h"

HistoryTopBarWidget::HistoryTopBarWidget(
	QWidget *parent,
	not_null<Window::Controller*> controller)
: RpWidget(parent)
, _controller(controller)
, _clearSelection(this, langFactory(lng_selected_clear), st::topBarClearButton)
, _forward(this, langFactory(lng_selected_forward), st::defaultActiveButton)
, _delete(this, langFactory(lng_selected_delete), st::defaultActiveButton)
, _back(this, st::historyTopBarBack)
, _call(this, st::topBarCall)
, _search(this, st::topBarSearch)
, _infoToggle(this, st::topBarInfo)
, _menuToggle(this, st::topBarMenuToggle)
, _onlineUpdater([this] { updateOnlineDisplay(); }) {
	subscribe(Lang::Current().updated(), [this] { refreshLang(); });
	setAttribute(Qt::WA_OpaquePaintEvent);

	_forward->setClickedCallback([this] { onForwardSelection(); });
	_forward->setWidthChangedCallback([this] { updateControlsGeometry(); });
	_delete->setClickedCallback([this] { onDeleteSelection(); });
	_delete->setWidthChangedCallback([this] { updateControlsGeometry(); });
	_clearSelection->setClickedCallback([this] { onClearSelection(); });
	_call->setClickedCallback([this] { onCall(); });
	_search->setClickedCallback([this] { onSearch(); });
	_menuToggle->setClickedCallback([this] { showMenu(); });
	_infoToggle->setClickedCallback([this] { toggleInfoSection(); });
	_back->addClickHandler([this] { backClicked(); });

	rpl::combine(
		_controller->historyPeer.value(),
		_controller->searchInPeer.value()
	) | rpl::combine_previous(
		std::make_tuple(nullptr, nullptr)
	) | rpl::map([](
			const std::tuple<PeerData*, PeerData*> &previous,
			const std::tuple<PeerData*, PeerData*> &current) {
		auto peer = std::get<0>(current);
		auto searchPeer = std::get<1>(current);
		auto peerChanged = (peer != std::get<0>(previous));
		auto searchInPeer
			= (peer != nullptr) && (peer == searchPeer);
		return std::make_tuple(searchInPeer, peerChanged);
	}) | rpl::start_with_next([this](
			bool searchInHistoryPeer,
			bool peerChanged) {
		auto animated = peerChanged
			? anim::type::instant
			: anim::type::normal;
		_search->setForceRippled(searchInHistoryPeer, animated);
	}, lifetime());

	subscribe(Adaptive::Changed(), [this] { updateAdaptiveLayout(); });
	if (Adaptive::OneColumn()) {
		createUnreadBadge();
	}
	subscribe(App::histories().sendActionAnimationUpdated(), [this](const Histories::SendActionAnimationUpdate &update) {
		if (update.history->peer == _historyPeer) {
			this->update();
		}
	});
	using UpdateFlag = Notify::PeerUpdate::Flag;
	auto flags = UpdateFlag::UserHasCalls
		| UpdateFlag::UserOnlineChanged
		| UpdateFlag::MembersChanged;
	subscribe(Notify::PeerUpdated(), Notify::PeerUpdatedHandler(flags, [this](const Notify::PeerUpdate &update) {
		if (update.flags & UpdateFlag::UserHasCalls) {
			if (update.peer->isUser()) {
				updateControlsVisibility();
			}
		} else {
			updateOnlineDisplay();
		}
	}));
	subscribe(Global::RefPhoneCallsEnabledChanged(), [this] {
		updateControlsVisibility();
	});

	rpl::combine(
		Auth().data().thirdSectionInfoEnabledValue(),
		Auth().data().tabbedReplacedWithInfoValue()
	) | rpl::start_with_next(
		[this] { updateInfoToggleActive(); },
		lifetime());

	setCursor(style::cur_pointer);
	updateControlsVisibility();
}

void HistoryTopBarWidget::refreshLang() {
	InvokeQueued(this, [this] { updateControlsGeometry(); });
}

void HistoryTopBarWidget::onForwardSelection() {
	if (App::main()) App::main()->forwardSelectedItems();
}

void HistoryTopBarWidget::onDeleteSelection() {
	if (App::main()) App::main()->confirmDeleteSelectedItems();
}

void HistoryTopBarWidget::onClearSelection() {
	if (App::main()) App::main()->clearSelectedItems();
}

void HistoryTopBarWidget::onSearch() {
	if (_historyPeer) {
		App::main()->searchInPeer(_historyPeer);
	}
}

void HistoryTopBarWidget::onCall() {
	if (auto user = _historyPeer->asUser()) {
		Calls::Current().startOutgoingCall(user);
	}
}

void HistoryTopBarWidget::showMenu() {
	if (!_historyPeer || _menu) {
		return;
	}
	_menu.create(parentWidget());
	_menu->setHiddenCallback([weak = make_weak(this), menu = _menu.data()] {
		menu->deleteLater();
		if (weak && weak->_menu == menu) {
			weak->_menu = nullptr;
			weak->_menuToggle->setForceRippled(false);
		}
	});
	_menu->setShowStartCallback(base::lambda_guarded(this, [this, menu = _menu.data()] {
		if (_menu == menu) {
			_menuToggle->setForceRippled(true);
		}
	}));
	_menu->setHideStartCallback(base::lambda_guarded(this, [this, menu = _menu.data()] {
		if (_menu == menu) {
			_menuToggle->setForceRippled(false);
		}
	}));
	_menuToggle->installEventFilter(_menu);
	Window::FillPeerMenu(
		_controller,
		_historyPeer,
		[this](const QString &text, base::lambda<void()> callback) {
			return _menu->addAction(text, std::move(callback));
		},
		Window::PeerMenuSource::History);
	_menu->moveToRight((parentWidget()->width() - width()) + st::topBarMenuPosition.x(), st::topBarMenuPosition.y());
	_menu->showAnimated(Ui::PanelAnimation::Origin::TopRight);
}

void HistoryTopBarWidget::toggleInfoSection() {
	if (Adaptive::ThreeColumn()
		&& (Auth().data().thirdSectionInfoEnabled()
			|| Auth().data().tabbedReplacedWithInfo())) {
		_controller->closeThirdSection();
	} else if (_historyPeer) {
		if (_controller->canShowThirdSection()) {
			Auth().data().setThirdSectionInfoEnabled(true);
			Auth().saveDataDelayed();
			if (Adaptive::ThreeColumn()) {
				_controller->showSection(
					Info::Memento::Default(_historyPeer),
					Window::SectionShow().withThirdColumn());
			} else {
				_controller->resizeForThirdSection();
				_controller->updateColumnLayout();
			}
		} else {
			_controller->showSection(Info::Memento(_historyPeer->id));
		}
	} else {
		updateControlsVisibility();
	}
}

bool HistoryTopBarWidget::eventFilter(QObject *obj, QEvent *e) {
	if (obj == _membersShowArea) {
		switch (e->type()) {
		case QEvent::MouseButtonPress:
			mousePressEvent(static_cast<QMouseEvent*>(e));
			return true;

		case QEvent::Enter:
			_membersShowAreaActive.fire(true);
			break;

		case QEvent::Leave:
			_membersShowAreaActive.fire(false);
			break;
		}
	}
	return TWidget::eventFilter(obj, e);
}

void HistoryTopBarWidget::paintEvent(QPaintEvent *e) {
	if (_animationMode) {
		return;
	}
	Painter p(this);

	auto ms = getms();
	_forward->stepNumbersAnimation(ms);
	_delete->stepNumbersAnimation(ms);
	auto hasSelected = (_selectedCount > 0);
	auto selectedButtonsTop = countSelectedButtonsTop(_selectedShown.current(getms(), hasSelected ? 1. : 0.));

	p.fillRect(QRect(0, 0, width(), st::topBarHeight), st::topBarBg);
	if (selectedButtonsTop < 0) {
		p.translate(0, selectedButtonsTop + st::topBarHeight);
		paintTopBar(p, ms);
	}
}

void HistoryTopBarWidget::paintTopBar(Painter &p, TimeMs ms) {
	auto history = App::historyLoaded(_historyPeer);
	if (!history) return;

	auto nameleft = _leftTaken;
	auto nametop = st::topBarArrowPadding.top();
	auto statustop = st::topBarHeight - st::topBarArrowPadding.bottom() - st::dialogsTextFont->height;
	auto namewidth = width() - _rightTaken - nameleft;

	p.setPen(st::dialogsNameFg);
	if (_historyPeer->isSelf()) {
		auto text = lang(lng_saved_messages);
		auto textWidth = st::historySavedFont->width(text);
		if (namewidth < textWidth) {
			text = st::historySavedFont->elided(text, namewidth);
		}
		p.setFont(st::historySavedFont);
		p.drawTextLeft(
			nameleft,
			(height() - st::historySavedFont->height) / 2,
			width(),
			text);
	} else {
		_historyPeer->dialogName().drawElided(p, nameleft, nametop, namewidth);

		p.setFont(st::dialogsTextFont);
		if (!history->paintSendAction(p, nameleft, statustop, namewidth, width(), st::historyStatusFgTyping, ms)) {
			auto statustext = _titlePeerText;
			auto statuswidth = _titlePeerTextWidth;
			if (statuswidth > namewidth) {
				statustext = st::dialogsTextFont->elided(
					statustext,
					namewidth,
					Qt::ElideLeft);
				statuswidth = st::dialogsTextFont->width(statustext);
			}
			p.setPen(_titlePeerTextOnline
				? st::historyStatusFgActive
				: st::historyStatusFg);
			p.drawTextLeft(nameleft, statustop, width(), statustext, statuswidth);
		}
	}
}

QRect HistoryTopBarWidget::getMembersShowAreaGeometry() const {
	int membersTextLeft = _leftTaken;
	int membersTextTop = st::topBarHeight - st::topBarArrowPadding.bottom() - st::dialogsTextFont->height;
	int membersTextWidth = _titlePeerTextWidth;
	int membersTextHeight = st::topBarHeight - membersTextTop;

	return myrtlrect(membersTextLeft, membersTextTop, membersTextWidth, membersTextHeight);
}

void HistoryTopBarWidget::mousePressEvent(QMouseEvent *e) {
	auto handleClick = (e->button() == Qt::LeftButton)
		&& (e->pos().y() < st::topBarHeight)
		&& (!_selectedCount);
	if (handleClick) {
		if (_animationMode && _back->rect().contains(e->pos())) {
			backClicked();
		} else if (_historyPeer) {
			infoClicked();
		}
	}
}

void HistoryTopBarWidget::infoClicked() {
	if (_historyPeer && _historyPeer->isSelf()) {
		_controller->showSection(Info::Memento(
			_historyPeer->id,
			Info::Section(Storage::SharedMediaType::Photo)));
	} else {
		_controller->showPeerInfo(_historyPeer);
	}
}

void HistoryTopBarWidget::backClicked() {
	_controller->showBackFromStack();
}

void HistoryTopBarWidget::setHistoryPeer(PeerData *historyPeer) {
	if (_historyPeer != historyPeer) {
		_historyPeer = historyPeer;
		_back->clearState();
		update();

		updateUnreadBadge();
		if (_historyPeer) {
			_info.create(
				this,
				_controller,
				_historyPeer,
				Ui::UserpicButton::Role::Custom,
				st::topBarInfoButton);
			_info->showSavedMessagesOnSelf(true);
			_info->setAttribute(Qt::WA_TransparentForMouseEvents);
		}
		if (_menu) {
			_menuToggle->removeEventFilter(_menu);
			_menu->hideFast();
		}
		updateOnlineDisplay();
		updateControlsVisibility();
	}
}

void HistoryTopBarWidget::resizeEvent(QResizeEvent *e) {
	updateControlsGeometry();
}

int HistoryTopBarWidget::countSelectedButtonsTop(float64 selectedShown) {
	return (1. - selectedShown) * (-st::topBarHeight);
}

void HistoryTopBarWidget::updateControlsGeometry() {
	auto hasSelected = (_selectedCount > 0);
	auto selectedButtonsTop = countSelectedButtonsTop(_selectedShown.current(hasSelected ? 1. : 0.));
	auto otherButtonsTop = selectedButtonsTop + st::topBarHeight;
	auto buttonsLeft = st::topBarActionSkip + (Adaptive::OneColumn() ? 0 : st::lineWidth);
	auto buttonsWidth = _forward->contentWidth() + _delete->contentWidth() + _clearSelection->width();
	buttonsWidth += buttonsLeft + st::topBarActionSkip * 3;

	auto widthLeft = qMin(width() - buttonsWidth, -2 * st::defaultActiveButton.width);
	_forward->setFullWidth(-(widthLeft / 2));
	_delete->setFullWidth(-(widthLeft / 2));

	selectedButtonsTop += (height() - _forward->height()) / 2;

	_forward->moveToLeft(buttonsLeft, selectedButtonsTop);
	if (!_forward->isHidden()) {
		buttonsLeft += _forward->width() + st::topBarActionSkip;
	}

	_delete->moveToLeft(buttonsLeft, selectedButtonsTop);
	_clearSelection->moveToRight(st::topBarActionSkip, selectedButtonsTop);

	if (_unreadBadge) {
		_unreadBadge->setGeometryToLeft(
			0,
			otherButtonsTop + st::titleUnreadCounterTop,
			_back->width(),
			st::dialogsUnreadHeight);
	}
	if (_back->isHidden()) {
		_leftTaken = st::topBarArrowPadding.right();
	} else {
		_leftTaken = 0;
		_back->moveToLeft(_leftTaken, otherButtonsTop);
		_leftTaken += _back->width();
		if (_info && !_info->isHidden()) {
			_info->moveToLeft(_leftTaken, otherButtonsTop);
			_leftTaken += _info->width();
		}
	}

	_rightTaken = 0;
	_menuToggle->moveToRight(_rightTaken, otherButtonsTop);
	_rightTaken += _menuToggle->width() + st::topBarSkip;
	_infoToggle->moveToRight(_rightTaken, otherButtonsTop);
	if (!_infoToggle->isHidden()) {
		_rightTaken += _infoToggle->width() + st::topBarSkip;
	}
	_search->moveToRight(_rightTaken, otherButtonsTop);
	_rightTaken += _search->width() + st::topBarCallSkip;
	_call->moveToRight(_rightTaken, otherButtonsTop);
	_rightTaken += _call->width();

	updateMembersShowArea();
}

void HistoryTopBarWidget::finishAnimating() {
	_selectedShown.finish();
	updateControlsVisibility();
}

void HistoryTopBarWidget::setAnimationMode(bool enabled) {
	if (_animationMode != enabled) {
		_animationMode = enabled;
		setAttribute(Qt::WA_OpaquePaintEvent, !_animationMode);
		finishAnimating();
		updateControlsVisibility();
	}
}

void HistoryTopBarWidget::updateControlsVisibility() {
	if (_animationMode) {
		hideChildren();
		return;
	}
	_clearSelection->show();
	_delete->setVisible(_canDelete);
	_forward->setVisible(_canForward);

	auto backVisible = Adaptive::OneColumn()
		|| (App::main() && !App::main()->stackIsEmpty());
	_back->setVisible(backVisible);
	if (_info) {
		_info->setVisible(Adaptive::OneColumn());
	}
	if (_unreadBadge) {
		_unreadBadge->show();
	}
	_search->show();
	_menuToggle->show();
	_infoToggle->setVisible(!Adaptive::OneColumn()
		&& _controller->canShowThirdSection());
	auto callsEnabled = false;
	if (auto user = _historyPeer ? _historyPeer->asUser() : nullptr) {
		callsEnabled = Global::PhoneCallsEnabled() && user->hasCalls();
	}
	_call->setVisible(callsEnabled);

	if (_membersShowArea) {
		_membersShowArea->show();
	}
	updateControlsGeometry();
}

void HistoryTopBarWidget::updateMembersShowArea() {
	if (!App::main()) {
		return;
	}
	auto membersShowAreaNeeded = [this]() {
		auto peer = App::main()->peer();
		if ((_selectedCount > 0) || !peer) {
			return false;
		}
		if (auto chat = peer->asChat()) {
			return chat->amIn();
		}
		if (auto megagroup = peer->asMegagroup()) {
			return megagroup->canViewMembers() && (megagroup->membersCount() < Global::ChatSizeMax());
		}
		return false;
	};
	if (!membersShowAreaNeeded()) {
		if (_membersShowArea) {
			_membersShowAreaActive.fire(false);
			_membersShowArea.destroy();
		}
		return;
	} else if (!_membersShowArea) {
		_membersShowArea.create(this);
		_membersShowArea->show();
		_membersShowArea->installEventFilter(this);
	}
	_membersShowArea->setGeometry(getMembersShowAreaGeometry());
}

void HistoryTopBarWidget::showSelected(SelectedState state) {
	auto canDelete = (state.count > 0 && state.count == state.canDeleteCount);
	auto canForward = (state.count > 0 && state.count == state.canForwardCount);
	if (_selectedCount == state.count && _canDelete == canDelete && _canForward == canForward) {
		return;
	}
	if (state.count == 0) {
		// Don't change the visible buttons if the selection is cancelled.
		canDelete = _canDelete;
		canForward = _canForward;
	}

	auto wasSelected = (_selectedCount > 0);
	_selectedCount = state.count;
	if (_selectedCount > 0) {
		_forward->setNumbersText(_selectedCount);
		_delete->setNumbersText(_selectedCount);
		if (!wasSelected) {
			_forward->finishNumbersAnimation();
			_delete->finishNumbersAnimation();
		}
	}
	auto hasSelected = (_selectedCount > 0);
	if (_canDelete != canDelete || _canForward != canForward) {
		_canDelete = canDelete;
		_canForward = canForward;
		updateControlsVisibility();
	}
	if (wasSelected != hasSelected) {
		setCursor(hasSelected ? style::cur_default : style::cur_pointer);

		updateMembersShowArea();
		_selectedShown.start(
			[this] { selectedShowCallback(); },
			hasSelected ? 0. : 1.,
			hasSelected ? 1. : 0.,
			st::slideWrapDuration,
			anim::easeOutCirc);
	} else {
		updateControlsGeometry();
	}
}

void HistoryTopBarWidget::selectedShowCallback() {
	updateControlsGeometry();
	update();
}

void HistoryTopBarWidget::updateAdaptiveLayout() {
	updateControlsVisibility();
	if (Adaptive::OneColumn()) {
		createUnreadBadge();
	} else if (_unreadBadge) {
		unsubscribe(base::take(_unreadCounterSubscription));
		_unreadBadge.destroy();
	}
	updateInfoToggleActive();
}

void HistoryTopBarWidget::createUnreadBadge() {
	if (_unreadBadge) {
		return;
	}
	_unreadBadge.create(this);
	_unreadBadge->setGeometryToLeft(0, st::titleUnreadCounterTop, _back->width(), st::dialogsUnreadHeight);
	_unreadBadge->show();
	_unreadBadge->setAttribute(Qt::WA_TransparentForMouseEvents);
	_unreadCounterSubscription = subscribe(
		Global::RefUnreadCounterUpdate(),
		[this] { updateUnreadBadge(); });
	updateUnreadBadge();
}

void HistoryTopBarWidget::updateUnreadBadge() {
	if (!_unreadBadge) return;

	auto mutedCount = App::histories().unreadMutedCount();
	auto fullCounter = App::histories().unreadBadge()
		+ (Global::IncludeMuted() ? 0 : mutedCount);

	// Do not include currently shown chat in the top bar unread counter.
	if (auto historyShown = App::historyLoaded(_historyPeer)) {
		auto shownUnreadCount = historyShown->unreadCount();
		fullCounter -= shownUnreadCount;
		if (historyShown->mute()) {
			mutedCount -= shownUnreadCount;
		}
	}

	auto active = (mutedCount < fullCounter);
	_unreadBadge->setText([&] {
		if (auto counter = (fullCounter - (Global::IncludeMuted() ? 0 : mutedCount))) {
			return (counter > 999)
				? qsl("..%1").arg(counter % 100, 2, 10, QChar('0'))
				: QString::number(counter);
		}
		return QString();
	}(), active);
}

void HistoryTopBarWidget::updateInfoToggleActive() {
	auto infoThirdActive = Adaptive::ThreeColumn()
		&& (Auth().data().thirdSectionInfoEnabled()
			|| Auth().data().tabbedReplacedWithInfo());
	auto iconOverride = infoThirdActive
		? &st::topBarInfoActive
		: nullptr;
	auto rippleOverride = infoThirdActive
		? &st::lightButtonBgOver
		: nullptr;
	_infoToggle->setIconOverride(iconOverride, iconOverride);
	_infoToggle->setRippleColorOverride(rippleOverride);
}

void HistoryTopBarWidget::updateOnlineDisplay() {
	if (!_historyPeer) return;

	QString text;
	const auto now = unixtime();
	bool titlePeerTextOnline = false;
	if (const auto user = _historyPeer->asUser()) {
		text = Data::OnlineText(user, now);
		titlePeerTextOnline = Data::OnlineTextActive(user, now);
	} else if (const auto chat = _historyPeer->asChat()) {
		if (!chat->amIn()) {
			text = lang(lng_chat_status_unaccessible);
		} else if (chat->participants.empty()) {
			if (!_titlePeerText.isEmpty()) {
				text = _titlePeerText;
			} else if (chat->count <= 0) {
				text = lang(lng_group_status);
			} else {
				text = lng_chat_status_members(lt_count, chat->count);
			}
		} else {
			auto online = 0;
			auto onlyMe = true;
			for (auto [user, v] : chat->participants) {
				if (user->onlineTill > now) {
					++online;
					if (onlyMe && user != App::self()) onlyMe = false;
				}
			}
			if (online > 0 && !onlyMe) {
				auto membersCount = lng_chat_status_members(lt_count, chat->participants.size());
				auto onlineCount = lng_chat_status_online(lt_count, online);
				text = lng_chat_status_members_online(lt_members_count, membersCount, lt_online_count, onlineCount);
			} else if (chat->participants.size() > 0) {
				text = lng_chat_status_members(lt_count, chat->participants.size());
			} else {
				text = lang(lng_group_status);
			}
		}
	} else if (auto channel = _historyPeer->asChannel()) {
		if (channel->isMegagroup() && channel->membersCount() > 0 && channel->membersCount() <= Global::ChatSizeMax()) {
			if (channel->mgInfo->lastParticipants.empty() || channel->lastParticipantsCountOutdated()) {
				Auth().api().requestLastParticipants(channel);
			}
			auto online = 0;
			bool onlyMe = true;
			for (auto &participant : std::as_const(channel->mgInfo->lastParticipants)) {
				if (participant->onlineTill > now) {
					++online;
					if (onlyMe && participant != App::self()) {
						onlyMe = false;
					}
				}
			}
			if (online && !onlyMe) {
				auto membersCount = lng_chat_status_members(lt_count, channel->membersCount());
				auto onlineCount = lng_chat_status_online(lt_count, online);
				text = lng_chat_status_members_online(lt_members_count, membersCount, lt_online_count, onlineCount);
			} else if (channel->membersCount() > 0) {
				text = lng_chat_status_members(lt_count, channel->membersCount());
			} else {
				text = lang(lng_group_status);
			}
		} else if (channel->membersCount() > 0) {
			text = lng_chat_status_members(lt_count, channel->membersCount());
		} else {
			text = lang(channel->isMegagroup() ? lng_group_status : lng_channel_status);
		}
	}
	if (_titlePeerText != text) {
		_titlePeerText = text;
		_titlePeerTextOnline = titlePeerTextOnline;
		_titlePeerTextWidth = st::dialogsTextFont->width(_titlePeerText);
		updateMembersShowArea();
		update();
	}
	updateOnlineDisplayTimer();
}

void HistoryTopBarWidget::updateOnlineDisplayTimer() {
	if (!_historyPeer) return;

	const auto now = unixtime();
	auto minTimeout = TimeMs(86400);
	const auto handleUser = [&](not_null<UserData*> user) {
		auto hisTimeout = Data::OnlineChangeTimeout(user, now);
		accumulate_min(minTimeout, hisTimeout);
	};
	if (const auto user = _historyPeer->asUser()) {
		handleUser(user);
	} else if (auto chat = _historyPeer->asChat()) {
		for (auto [user, v] : chat->participants) {
			handleUser(user);
		}
	} else if (_historyPeer->isChannel()) {
	}
	updateOnlineDisplayIn(minTimeout);
}

void HistoryTopBarWidget::updateOnlineDisplayIn(TimeMs timeout) {
	_onlineUpdater.callOnce(timeout);
}
