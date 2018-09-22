/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "ui/rp_widget.h"
#include "base/timer.h"

namespace Ui {
class UserpicButton;
class RoundButton;
class IconButton;
class DropdownMenu;
class UnreadBadge;
} // namespace Ui

namespace Window {
class Controller;
} // namespace Window

class HistoryTopBarWidget : public Ui::RpWidget, private base::Subscriber {
public:
	HistoryTopBarWidget(
		QWidget *parent,
		not_null<Window::Controller*> controller);

	struct SelectedState {
		bool textSelected = false;
		int count = 0;
		int canDeleteCount = 0;
		int canForwardCount = 0;
	};

	void updateControlsVisibility();
	void finishAnimating();
	void showSelected(SelectedState state);
	rpl::producer<bool> membersShowAreaActive() const {
		return _membersShowAreaActive.events();
	}
	void setAnimationMode(bool enabled);

	void setHistoryPeer(PeerData *historyPeer);

	static void paintUnreadCounter(
		Painter &p,
		int outerWidth,
		PeerData *substractPeer = nullptr);

protected:
	void paintEvent(QPaintEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;
	bool eventFilter(QObject *obj, QEvent *e) override;

private:
	void refreshLang();
	void updateControlsGeometry();
	void selectedShowCallback();
	void updateInfoToggleActive();

	void onForwardSelection();
	void onDeleteSelection();
	void onClearSelection();
	void onCall();
	void onSearch();
	void showMenu();
	void toggleInfoSection();

	void updateAdaptiveLayout();
	int countSelectedButtonsTop(float64 selectedShown);

	void paintTopBar(Painter &p, TimeMs ms);
	QRect getMembersShowAreaGeometry() const;
	void updateMembersShowArea();
	void updateOnlineDisplay();
	void updateOnlineDisplayTimer();
	void updateOnlineDisplayIn(TimeMs timeout);

	void infoClicked();
	void backClicked();

	void createUnreadBadge();
	void updateUnreadBadge();

	not_null<Window::Controller*> _controller;
	PeerData *_historyPeer = nullptr;

	int _selectedCount = 0;
	bool _canDelete = false;
	bool _canForward = false;

	Animation _selectedShown;

	object_ptr<Ui::RoundButton> _clearSelection;
	object_ptr<Ui::RoundButton> _forward, _delete;

	object_ptr<Ui::IconButton> _back;
	object_ptr<Ui::UnreadBadge> _unreadBadge = { nullptr };
	object_ptr<Ui::UserpicButton> _info = { nullptr };

	object_ptr<Ui::IconButton> _call;
	object_ptr<Ui::IconButton> _search;
	object_ptr<Ui::IconButton> _infoToggle;
	object_ptr<Ui::IconButton> _menuToggle;
	object_ptr<Ui::DropdownMenu> _menu = { nullptr };

	object_ptr<TWidget> _membersShowArea = { nullptr };
	rpl::event_stream<bool> _membersShowAreaActive;

	QString _titlePeerText;
	bool _titlePeerTextOnline = false;
	int _titlePeerTextWidth = 0;
	int _leftTaken = 0;
	int _rightTaken = 0;
	bool _animationMode = false;

	int _unreadCounterSubscription = 0;
	base::Timer _onlineUpdater;

};
