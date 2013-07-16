#include <sstream>
#include <algorithm>
#include <string>
#include "keys.h"
#include "geometry.h"
#include "commands.h"
#include "engine.h"
#include <ctype.h>
#include "player.h"
#include <SFML/Window.hpp>
#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <stdio.h>
#include <math.h>
#include "misc.h"

using namespace sf;

static bool translate_command_string(Command &desc, const char *cmd);
static bool translate_trigger_string(Trigger &trigger, const char *name);
static double trigger_value(int joyid, Window &window, const Trigger &trigger, double &value2);

static bool translate_command_string(Command &desc, const char *cmd0) {
	std::string item;
	std::string command;
	std::string direction;
	if (!cmd0) {
		fprintf(stderr, "Critical error: null command\n");
		return false;
	}

	std::istringstream in(LowerCaseString(cmd0));
	if (!(in >> item)) {
		return false;
	}
	if (!(item == "tank" || item == "canon")) {
		command = item;
		item = "tank";
	} else {
		if (!(in >> command)) {
			return false;
		}
	}
	if (item != "tank" && item != "canon") {
		return false;
	}
	if (command == "left" || command == "right" || command == "down" || command == "up" || command == "backward" || command == "forward" || command == "latleft" || command == "latright") {
		direction = command;
		command = (item == "tank" ? "move" : "direction");
	} else if (command != "lookat" && command != "shoot") {
		if (!(in >> direction)) return false;
	}
	/* now, set fields */
	/* Compute piece and relativity fields */
	if (command == "turn" || command == "direction" || command == "lookat") {
		desc.piece = item == "tank" ? CP_Tank_Orientation : CP_Canon_Orientation;
		if (command == "turn")        desc.relativity = CR_Relative;
		else if (command == "lookat") desc.relativity = CR_LookAt;
		else                          desc.relativity = CR_Absolute;
	} else if (command == "move") {
		if (item == "canon") return false;
		desc.piece = CP_Tank_Position;
		desc.relativity = CR_Relative;
	} else if (command == "shoot") {
		desc.piece = CP_Shooter;
		desc.relativity = CR_Absolute;
	}
	/* Compute axis and sens fields */
	if (command == "lookat" || command == "shoot") {
		desc.axis = CD_Axial;
		desc.sens = CD_Positive;
	} else {
		if (direction == "down" || direction == "up")    desc.axis = CD_Vertical;
		else if (direction == "left" || direction == "right") {
			if (command == "turn") desc.axis = CD_Rotate;
			else desc.axis = CD_Horizontal;
		}
		else if (direction == "forward" || direction == "backward") desc.axis = CD_Axial;
		else if (direction == "latleft" || direction == "latright") desc.axis = CD_Lateral;
		else return false;
		if (command == "turn" && desc.axis != CD_Rotate) return false;
		if (direction == "up" || direction == "left" || direction == "backward" || direction == "latleft")
			desc.sens = CD_Negative;
		else	desc.sens = CD_Positive;
	}
	return true;
}
static bool translate_trigger_string(Trigger &trigger, const char *name) {
	if (!name) {
		fprintf(stderr, "Critical error: null trigger\n");
		return false;
	}
	std::istringstream in(LowerCaseString(name));
	std::string item;
	std::string code;
	NamedKey *codemap = NULL;

	if (!(in >> item)) return false;
	if (item != "keyboard" && item != "mouse" && item != "key" && item != "joy") {
		code = item;
		item = "keyboard";
	} else {
		if (!(in >> code)) return false;
	}
	if (item == "key") item = "keyboard";
	if (item == "joy") item = "joystick";

	/* set trigger.type and codemap */
	if (item == "keyboard") {
		trigger.type = TT_KeyboardKey;
		codemap = key_codemap;
	} else if (item == "mouse") {
		if (code == "position") {
			trigger.type = TT_MousePosition;
			codemap = NULL;
		} else {
			trigger.type = TT_MouseButton;
			codemap = mouse_codemap;
		}
	} else if (item == "joystick") {
		/* example1: joystick axis 1 neg */
		/* example2: joystick button 1 */
		std::string clss = code;
		if (!(in >> code)) return false;
		if (clss != "axis" && clss != "button") return false;
		if (clss == "button") {
			trigger.type = TT_JoystickButton;
			codemap = joybutton_codemap;
		} else {
			codemap = joyaxis_codemap;
			std::string direc;
			if (!(in >> direc)) return false;
			if (direc == "left" || direc == "up" || direc == "neg") trigger.type = TT_JoystickAxisNeg;
			else if (direc == "right" || direc == "down" || direc == "pos") trigger.type = TT_JoystickAxisPos;
			else return false;
		}
	}
	if (codemap == NULL) {trigger.keycode = 0; return true;}
	for (NamedKey *p=codemap;p->name;p++) {
		if (LowerCaseString(p->name) == code) {
			trigger.keycode = p->keycode;
			return true;
		}
	}
	std::istringstream innum(code);
	if (innum >> trigger.keycode) return true;
	return false;
}
static double getJoyAxis(int joyid, Joystick::Axis axis) {
	if (joyid < 0) return 0;
	double v = Joystick::getAxisPosition(joyid, (Joystick::Axis)axis) / 100;
	if (fabs(v) <= 0.3) v = 0;
	return v;
}
static double trigger_value(int joyid, Window &window, const Trigger &trigger, double &value2) {
	value2 = 0;
	int keycode = trigger.keycode;
	if (trigger.type == TT_JoystickAxisNeg || trigger.type == TT_JoystickAxisPos) {
		double axis = getJoyAxis(joyid, (Joystick::Axis)keycode);
		if ((axis > 0) == (trigger.type == TT_JoystickAxisNeg)) return 0;
		else return fabs(axis);
	} else if (trigger.type == TT_JoystickButton) {
		if (joyid < 0) return 0;
		return Joystick::isButtonPressed(joyid, keycode);
	} else if (trigger.type == TT_KeyboardKey) {
		return Keyboard::isKeyPressed((Keyboard::Key)keycode);
	} else if (trigger.type == TT_MouseButton) {
		return Mouse::isButtonPressed((Mouse::Button)keycode);
	} else if (trigger.type == TT_MousePosition) {
		Vector2i pos = Mouse::getPosition(window);
		value2 = pos.y;
		return pos.x;
	}
	return 0;
}

static bool notzero(const Vector2d &vect) {
	return fabs(vect.x)+fabs(vect.y) >= 1e-3;
}
void KeymapController::mapControl(const char *trigger_name, const char *command_name) {
	TriggerCommand tc;
	if (!translate_trigger_string(tc.trigger, trigger_name)) {
		fprintf(stderr, "Invalid trigger name %s\n", trigger_name);
		return;
	}
	if (!translate_command_string(tc.command, command_name)) {
		fprintf(stderr, "Invalid command name %s\n", command_name);
		return;
	}
	commandmap.push_back(tc);
}
void KeymapController::reportPlayerMovement(Player *player, PlayerControllingData &pcd) {
	Window &window = player->getEngine()->getWindow();
	Vector2d rela[CP_Shooter+1]; /* CP_Tank_Orientation, CP_Tank_Position, CP_Canon_Orientation, CP_Shooter */
	Vector2d absa[CP_Shooter+1];
	bool defined_rela[CP_Shooter+1]={0};
	bool defined_absa[CP_Shooter+1]={0};
	bool defined_rotate[CP_Shooter+1]={0};
	double rotate[CP_Shooter+1]={0};

	double axial_angle = player->getTankAngle();
	Vector2d axial = Vector2d(cos(axial_angle), sin(axial_angle));
	Vector2d tank_position = player->getPosition();

	for(iterator it = commandmap.begin(); it != commandmap.end(); ++it) {
		double value = 0, value2 = 0;
		const Command &cmd = (*it).command;
		value = trigger_value(joyid, window, (*it).trigger, value2);
		if (cmd.sens == CD_Negative) {value = -value;value2 = -value2;}
		Vector2d &rel=rela[cmd.piece];
		Vector2d &abs=absa[cmd.piece];

		Vector2d &ar = (cmd.relativity == CR_Relative ? rel : abs);
		bool &dar = (cmd.relativity == CR_Relative ? defined_rela : defined_absa)[cmd.piece];
		if (cmd.relativity == CR_LookAt) {
			abs.x = value - tank_position.x;
			abs.y = value2 - tank_position.y;
			defined_absa[cmd.piece] = true;
		} else if (cmd.axis == CD_Horizontal) {
			ar.x += value;
			dar = true;
		} else if (cmd.axis == CD_Vertical) {
			ar.y += value;
			dar = true;
		} else if (cmd.axis == CD_Axial) {
			ar.x += value * axial.x;
			ar.y += value * axial.y;
			dar = true;
		} else if (cmd.axis == CD_Lateral) {
			ar.x -= value * axial.y;
			ar.y += value * axial.x;
			dar = true;
		} else if (cmd.axis == CD_Rotate) {
			rotate[cmd.piece] += value;
			defined_rotate[cmd.piece] = true;
		}
	}
	
	/* define turret & tank angles */
	for(unsigned i=0; i < 2; i++) {
		unsigned index = (i==0?CP_Tank_Orientation:CP_Canon_Orientation);
		if (notzero(absa[index])) {
			double angle = angle_from_dxdy(absa[index].x, absa[index].y);
			if (index) pcd.setCanonAngle(angle); else pcd.setTankAngle(angle);
		} else if (fabs(rotate[index])>1e-3) {
			double r = rotate[index];
			if (fabs(r) > 1) r = r/fabs(r);
			if (index) pcd.rotateCanon(r); else pcd.rotateTank(r);
		}
	}
	/* define tank position */
	if (defined_absa[CP_Tank_Position]) {
		pcd.setPosition(absa[CP_Tank_Position]);
	} else if (notzero(rela[CP_Tank_Position])) {
		Vector2d v = rela[CP_Tank_Position];
		if (vectorModule(v) >= 1) normalizeVector(v, 1); /* FIXME: Make vector module equal to 0.5 rather than 0.5*sqrt(2) when moving diagonally with half pressure on an analog stick */
		pcd.move(v);
	}
	if (notzero(absa[CP_Shooter]) || notzero(absa[CP_Shooter])) {
		pcd.keepShooting();
	}
	if (defined_absa[CP_Tank_Orientation] || defined_rela[CP_Tank_Orientation] || defined_rotate[CP_Tank_Orientation]) {
		pcd.preserveTankAngle();
	}
	if (!(defined_absa[CP_Canon_Orientation] || defined_rela[CP_Canon_Orientation] || defined_rotate[CP_Canon_Orientation])) {
		pcd.adaptCanonAngle();
	}
}

KeymapController::KeymapController(int joyid):joyid(joyid) {}
KeymapController *KeymapController::clone(int joyid1) {
	KeymapController *kmp = new KeymapController(joyid1);
	kmp->commandmap = commandmap;
	return kmp;
}

int KeymapController::getJoystickId(void) const {
	return joyid;
}

bool KeymapController::maybeConcerned(const Event &e) {
	return e.type == Event::JoystickButtonPressed || e.type == Event::JoystickMoved || e.type == Event::KeyPressed || e.type == Event::KeyReleased;
}
bool KeymapController::isConcerned(const Event &e, int &ojoyid) {
	bool isJoystickTemplate = false;
	if (commandmap.size() > 0) {
		unsigned type = commandmap[0].trigger.type;
		if (type == TT_JoystickButton || type == TT_JoystickAxisNeg || type == TT_JoystickAxisPos) isJoystickTemplate = true;
	}
	if (e.type == Event::JoystickButtonPressed) {
		ojoyid = e.joystickButton.joystickId;
		return isJoystickTemplate && (joyid == ojoyid || joyid == -1);
	} else if (e.type == Event::JoystickMoved) {
		ojoyid = e.joystickMove.joystickId;
		return isJoystickTemplate && (joyid == ojoyid || joyid == -1);
	} else if (e.type == Event::KeyPressed || e.type == Event::KeyReleased) {
		for(unsigned i=0; i < commandmap.size(); i++) {
			if (commandmap[i].trigger.type == TT_KeyboardKey && e.key.code == commandmap[i].trigger.keycode)
				return true;
		}
	}
	return false;
}
