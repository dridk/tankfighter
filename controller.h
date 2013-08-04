#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__
#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Window.hpp>

class Player;
class Missile;
class MissileControllingData;
class PlayerControllingData;

class Controller
{
	public:
	virtual void reportMissileMovement(Missile *missile, MissileControllingData &mcd);
	virtual void reportPlayerMovement(Player *player, PlayerControllingData &pcd)=0;
	virtual bool missileCreation(Missile *ml);
	virtual bool missileCollision(Missile *ml, Player *other);
	virtual void teleported(void);
	Controller();
	virtual ~Controller();
	void    setPlayer(Player *player);
	Player *getPlayer(void) const;
	private:
	Player *player;
};

class LocalController:public Controller
{
	public:
	virtual bool isConcerned(const sf::Event &e) const;
};

enum JoystickAxis {HorizontalMove, VerticalMove, HorizontalDirection, VerticalDirection};
class JoystickController: public LocalController
{
	public:
	JoystickController(int joyid);
	virtual void reportPlayerMovement(Player *player, PlayerControllingData &pcd);
	virtual bool isConcerned(const sf::Event &e) const;
	int getJoystickId(void) const;
	private:
	float getJoyAxis(sf::Joystick::Axis axis);
	float getAxis(JoystickAxis axis);
	bool getButton(int id);
	bool is_shooting();
	unsigned char joyid;
	unsigned char joytype;
	unsigned char flags;
};

class KeyboardMouseController: public Controller
{
	public:
	virtual void reportPlayerMovement(Player *player, PlayerControllingData &pcd);
};

#endif
