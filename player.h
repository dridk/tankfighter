#ifndef __PLAYER_H__
#define __PLAYER_H__
#include "coretypes.h"
#include "entity.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System.hpp>
#include <SFML/System/Clock.hpp>

class Controller;
class Engine;

class Player: public Entity
{
	public:
	Player(Controller *controller, Engine *engine);
	~Player();
	virtual Vector2d getSize() const;
	virtual void draw(sf::RenderTarget &target) const;
	virtual Vector2d movement(sf::Int64 tm);
	virtual void event_received(EngineEvent *event);
	
	sf::Color getColor(void);
	double getCanonAngle(void);
	double getTankAngle(void);
	Vector2d getPosition(void);

	void setCanonAngle(float angle);
	void rotateCanon(float angleSpeed);

	void preserveTankAngle(void);
	void adaptCanonAngle(void);
	void setTankAngle(float angle);
	void rotateTank (float angleSpeed);

	void move(Vector2d speed);
	void setPosition(Vector2d position);

	void keepShooting(void);
	int  getScore(void);
	void setScore(int score);

	void killedBy(Player *player);
	void killedPlayer(Player *player);

	Controller *getController(void);

	private:
	static const short maxMissileCount = 3;
	void teleport(void);
	Controller *controller;
	static const float missileDelay; /* milliseconds */
	float tank_direction;
	float canon_direction;
	sf::Color color;
	sf::Clock shoot_clock;

	int playerUID;
	static int UID;
	int score;
	short missileCount;

/* dynamic info */
	bool adapt_canon_angle;
	bool preserve_tank_angle;
	bool teleporting;
	bool is_shooting;
	double canon_rotation, tank_rotation;
	Vector2d tank_movement;
	Vector2d tank_goto;
	void try_shoot(void);
	sf::Sprite &getSprite(const char *name) const;
	void computeRandomPosition(void);
};
#endif
