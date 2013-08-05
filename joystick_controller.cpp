#include "controller.h"
#include "player.h"
#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Window.hpp>
#include <SFML/Window/Event.hpp>
#include "geometry.h"
#include <math.h>
#include <stdio.h>
#include "parameters.h"
#include "engine.h"

using namespace sf;


enum JoystickFlags {JF_ANALOG_USED=1, JF_DV_USED=2, JF_DH_USED=4, JF_SIGNALED=8};
enum JoystickType {JT_UNKNOWN, JT_XYZR, JT_XYUV, JT_XYUZ, JT_XYUR, JT_XBOX, JT_XY, JT_XYUZR};
JoystickController::JoystickController(int joyid0):joyid(joyid0) {
	flags=0;
	joytype = JT_UNKNOWN;
	bool hasZ = Joystick::hasAxis(joyid, Joystick::Z);
	bool hasR = Joystick::hasAxis(joyid, Joystick::R);
	bool hasU = Joystick::hasAxis(joyid, Joystick::U);
	bool hasV = Joystick::hasAxis(joyid, Joystick::V);
	bool hasZR = hasZ && hasR;
	bool hasUV = hasU && hasV;
	if (!hasZR && !hasUV) joytype = JT_XY;
	if (hasZR && !hasUV) joytype = JT_XYZR;
	if (hasUV && !hasZR) joytype = JT_XYUV;
	if (hasU && hasZR) joytype = JT_XYUZR;
	if (hasZ && hasU && !hasV && !hasR) joytype = JT_XYUZ;
	if (hasR && hasU && !hasV && !hasZ) joytype = JT_XYUR;
	if (fabs(fabs(getJoyAxis(Joystick::Z))-1)<1e-3 && hasUV) {
		joytype = JT_XBOX;
	}
#ifdef DEBUG_JOYSTICK
	fprintf(stderr, "joytype ZRUV=%d%d%d%d %d\n", hasZ, hasR, hasU, hasV, joytype);
#endif
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
bool JoystickController::getButton(int id) {
	return Joystick::isButtonPressed(joyid, id);
}
float JoystickController::getAxis(JoystickAxis axis) {
	switch(axis) {
		case HorizontalMove:
			return getJoyAxis(Joystick::X)+getJoyAxis(Joystick::PovX);
		case VerticalMove:
			return getJoyAxis(Joystick::Y)+getJoyAxis(Joystick::PovY);
		case HorizontalDirection:
			{
			int hd=getButton(1)-getButton(3);
			double ha;
			if (joytype == JT_XYZR) ha=getJoyAxis(Joystick::Z);
			else ha=getJoyAxis(Joystick::U);
			if (hd) flags|=JF_DH_USED;
			if (fabs(ha)>=1e-3) flags|=JF_ANALOG_USED;
			return ha+hd;
			}
		case VerticalDirection:
			{
			int vd=getButton(2)-getButton(0);
			double va=0;
			if (joytype == JT_XBOX) va=getJoyAxis(Joystick::V);
			if (joytype == JT_XYZR) va=getJoyAxis(Joystick::R);
			if (joytype == JT_XYUR) va=getJoyAxis(Joystick::R);
			if (joytype == JT_XYUV) va=getJoyAxis(Joystick::V);
			if (joytype == JT_XYUZ) va=getJoyAxis(Joystick::Z);
			if (joytype == JT_XYUZR) va=getJoyAxis(Joystick::Z)+getJoyAxis(Joystick::R);
			if (vd) flags|=JF_DV_USED;
			if (fabs(va)>=1e-3) flags|=JF_ANALOG_USED;
			return va+vd;
			}
		default:
		return 0;
	}
}

bool JoystickController::is_shooting() {
	for(unsigned i=4; i < Joystick::getButtonCount(joyid); i++) {
		if (Joystick::isButtonPressed(joyid, i)) {
			return true;
		}
	}
	if (joytype == JT_XBOX) { /* Xbox controller */
		if (getJoyAxis(Joystick::Z) > 1e-3 || getJoyAxis(Joystick::R) > 1e-3) return true;
	}
	return false;
}
void JoystickController::reportPlayerMovement(Player *player, PlayerControllingData &pcd) {
	bool moving = false;
	if (is_shooting()) pcd.keepShooting();
	Vector2d mv = Vector2d(getAxis(HorizontalMove), getAxis(VerticalMove));
	Vector2d ca = Vector2d(getAxis(HorizontalDirection), getAxis(VerticalDirection));
	if (sqrt(mv.x*mv.x+mv.y*mv.y) >= parameters.joyTankSpeedCalibration()) {
		normalizeVector(mv, 1);
		pcd.move(mv);
		moving = true;
	}
	if (sqrt(ca.x*ca.x+ca.y*ca.y) >= parameters.joyCanonDirectionCalibration()) {
		pcd.setCanonAngle(angle_from_dxdy(ca.x, ca.y));
	} else if (moving) {
		pcd.setCanonAngle(angle_from_dxdy(mv.x, mv.y));
	}
	if ((flags & (JF_DH_USED | JF_DV_USED))==(JF_DH_USED | JF_DV_USED) && !(flags & JF_ANALOG_USED) && joytype!=JT_XY && joytype!=JT_UNKNOWN && !(flags & JF_SIGNALED)) {
		flags|=JF_SIGNALED;
		player->getEngine()->display("Select analog mode to get more accurate canon direction");
	}
}

int JoystickController::getJoystickId(void) const {
	return joyid;
}

bool JoystickController::isConcerned(const Event &e) const {
	int ojoyid = -1;
	if (e.type == Event::JoystickButtonPressed) {
		ojoyid = e.joystickButton.joystickId;
	} else if (e.type == Event::JoystickMoved) {
		ojoyid = e.joystickMove.joystickId;
	} else return false;
	return ojoyid == joyid;
}
