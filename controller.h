#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__
#include <SFML/Window/Joystick.hpp>

class Player;

class Controller
{
	public:
	virtual void detectMovement(Player *player)=0;
	virtual ~Controller();
};

enum JoystickAxis {HorizontalMove, VerticalMove, HorizontalDirection, VerticalDirection};
class JoystickController: public Controller
{
	public:
	JoystickController(int joyid);
	virtual void detectMovement(Player *player);
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
	virtual void detectMovement(Player *player);
};

#endif
