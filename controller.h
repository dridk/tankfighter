#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

class Player;

class Controller
{
	public:
	virtual void detectMovement(Player *player)=0;
	virtual ~Controller();
};

class JoystickController: public Controller
{
	public:
	virtual void detectMovement(Player *player);
	private:
	short joyid;
};

class KeyboardMouseController: public Controller
{
	public:
	virtual void detectMovement(Player *player);
};
#endif
