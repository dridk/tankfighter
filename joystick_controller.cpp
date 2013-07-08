#include "controller.h"
#include "player.h"
#include <SFML/Window/Joystick.hpp>
#include "geometry.h"
#include <math.h>
#include <stdio.h>

using namespace sf;


enum JoystickType {JT_UNKNOWN, JT_XYZR, JT_XYUV, JT_XYUZ, JT_XBOX};
JoystickController::JoystickController(int joyid0):joyid(joyid0) {
	joytype = JT_UNKNOWN;
	bool hasZR = Joystick::hasAxis(joyid, Joystick::Z) && Joystick::hasAxis(joyid, Joystick::R);
	bool hasUV = Joystick::hasAxis(joyid, Joystick::U) && Joystick::hasAxis(joyid, Joystick::V);
	if (hasZR && !hasUV) joytype = JT_XYZR;
	if (hasUV && !hasZR) joytype = JT_XYUV;
	if (hasUV && hasZR) joytype = JT_XYUZ;
	if (fabs(fabs(getJoyAxis(Joystick::Z))-1)<1e-3) {
		joytype = JT_XBOX;
	}
}
float JoystickController::getJoyAxis(Joystick::Axis axis) {
	if (Joystick::hasAxis(joyid, axis))
		{
			double d = Joystick::getAxisPosition(joyid, axis);
			if (fabs(d) <= 100.1) return d/100;
			else return 0;
		}
	else return 0;
}
float JoystickController::getAxis(JoystickAxis axis) {
	switch(axis) {
		case HorizontalMove:
			return getJoyAxis(Joystick::X)+getJoyAxis(Joystick::PovX);
		case VerticalMove:
			return getJoyAxis(Joystick::Y)+getJoyAxis(Joystick::PovY);
		case HorizontalDirection:
			if (joytype == JT_XYZR) return getJoyAxis(Joystick::Z);
			else return getJoyAxis(Joystick::U); /* XBOX or XYUV or XYUZ */
		case VerticalDirection:
			{
			if (joytype == JT_XBOX) return getJoyAxis(Joystick::V);
			if (joytype == JT_XYZR) return getJoyAxis(Joystick::R);
			if (joytype == JT_XYUV) return getJoyAxis(Joystick::V);
			if (joytype == JT_XYUZ) return getJoyAxis(Joystick::Z);
			return 0;
			}
		default:
		return 0;
	}
}

bool JoystickController::is_shooting() {
	for(int i=0; i < Joystick::getButtonCount(joyid); i++) {
		if (Joystick::isButtonPressed(joyid, i)) {
			return true;
		}
	}
	if (joytype == JT_XBOX) { /* Xbox controller */
		if (getJoyAxis(Joystick::Z) > 1e-3 || getJoyAxis(Joystick::R) > 1e-3) return true;
	}
	return false;
}
void JoystickController::detectMovement(Player *player) {
	bool moving = false;
	if (is_shooting()) player->keepShooting();
	Vector2d mv = Vector2d(getAxis(HorizontalMove), getAxis(VerticalMove));
	Vector2d ca = Vector2d(getAxis(HorizontalDirection), getAxis(VerticalDirection));
	if (fabs(mv.x)+fabs(mv.y) >= 0.3) {
		normalizeVector(mv, 1);
		player->move(mv);
		moving = true;
	}
	double angle = angle_from_dxdy(ca.x, ca.y);
	if (fabs(ca.x)+fabs(ca.y) >= 0.3) player->setCanonAngle(angle_from_dxdy(ca.x, ca.y));
	else if (moving) {
		player->setCanonAngle(angle_from_dxdy(mv.x, mv.y));
	}
}

