/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Ui {
class RpWidget;
} // namespace Ui

namespace Window {

class Controller;

enum class PeerMenuSource {
	ChatsList,
	History,
	Profile,
};

using PeerMenuCallback = base::lambda<QAction*(
	const QString &text,
	base::lambda<void()> handler)>;

void FillPeerMenu(
	not_null<Controller*> controller,
	not_null<PeerData*> peer,
	const PeerMenuCallback &addAction,
	PeerMenuSource source);

void PeerMenuDeleteContact(not_null<UserData*> user);
void PeerMenuShareContactBox(not_null<UserData*> user);
void PeerMenuAddContact(not_null<UserData*> user);
void PeerMenuAddChannelMembers(not_null<ChannelData*> channel);

base::lambda<void()> ClearHistoryHandler(not_null<PeerData*> peer);
base::lambda<void()> DeleteAndLeaveHandler(not_null<PeerData*> peer);

QPointer<Ui::RpWidget> ShowForwardMessagesBox(
	MessageIdsList &&items,
	base::lambda_once<void()> &&successCallback = nullptr);

} // namespace Window
