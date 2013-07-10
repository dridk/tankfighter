#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__
#include <SFML/Window/Joystick.hpp>

class Player;
class Missile;
class MissileControllingData;
class PlayerControllingData;

class Controller
{
	public:
	virtual void reportMissileMovement(Missile *missile, MissileControllingData &mcd);
	virtual void reportPlayerMovement(Player *player, PlayerControllingData &pcd)=0;
	virtual ~Controller();
};

enum JoystickAxis {HorizontalMove, VerticalMove, HorizontalDirection, VerticalDirection};
class JoystickController: public Controller
{
	public:
	JoystickController(int joyid);
	virtual void reportPlayerMovement(Player *player, PlayerControllingData &pcd);
	int getJoystickId(void) const;
	private:
	float getJoyAxis(sf::Joystick::Axis axis);
	float getAxis(JoystickAxis axis);
	bool is_shooting();
	unsigned char joyid;
	unsigned char joytype;
};

class KeyboardMouseController: public Controller
{
	public:
	virtual void reportPlayerMovement(Player *player, PlayerControllingData &pcd);
};

#endif
