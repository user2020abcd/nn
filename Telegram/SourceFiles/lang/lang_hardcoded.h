/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

namespace Lang {
namespace Hard {

inline QString FavedSetTitle() {
	return qsl("Favorite stickers");
}

inline QString CallErrorIncompatible() {
	return qsl("{user}'s app is using an incompatible protocol. They need to update their app before you can call them.");
}

inline QString ServerError() {
	return qsl("Internal server error.");
}

inline QString ClearPathFailed() {
	return qsl("Clear failed :(");
}

} // namespace Hard
} // namespace Lang
