/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ui/wrap/fade_wrap.h"

#include "ui/widgets/shadow.h"

namespace Ui {

FadeWrap<RpWidget>::FadeWrap(
	QWidget *parent,
	object_ptr<RpWidget> &&child,
	float64 scale)
: Parent(parent, std::move(child))
, _animation(this, scale)
, _duration(st::fadeWrapDuration) {
	if (auto weak = wrapped()) {
		weak->show();
	}
}

FadeWrap<RpWidget> *FadeWrap<RpWidget>::setDuration(int duration) {
	_duration = duration;
	return this;
}

FadeWrap<RpWidget> *FadeWrap<RpWidget>::toggle(
		bool shown,
		anim::type animated) {
	auto changed = (shown != _animation.visible());
	if (shown) {
		if (animated == anim::type::normal) {
			_animation.fadeIn(_duration);
		} else {
			_animation.show();
		}
	} else {
		if (animated == anim::type::normal) {
			_animation.fadeOut(_duration);
		} else {
			_animation.hide();
		}
	}
	if (changed) {
		_toggledChanged.fire_copy(shown);
	}
	return this;
}

FadeWrap<RpWidget> *FadeWrap<RpWidget>::finishAnimating() {
	_animation.finish();
	return this;
}

FadeWrap<RpWidget> *FadeWrap<RpWidget>::toggleOn(
		rpl::producer<bool> &&shown) {
	std::move(
		shown
	) | rpl::start_with_next([this](bool shown) {
		toggle(shown, anim::type::normal);
	}, lifetime());
	finishAnimating();
	return this;
}

void FadeWrap<RpWidget>::paintEvent(QPaintEvent *e) {
	Painter p(this);
	_animation.paint(p);
}

FadeShadow::FadeShadow(QWidget *parent)
: FadeShadow(parent, st::shadowFg) {
}

FadeShadow::FadeShadow(QWidget *parent, style::color color)
: Parent(parent, object_ptr<PlainShadow>(parent, color)) {
	hide(anim::type::instant);
}

} // namespace Ui

