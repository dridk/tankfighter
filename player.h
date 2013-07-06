#ifndef __PLAYER_H__
#define __PLAYER_H__
#include "coretypes.h"
#include "entity.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System.hpp>
#include <SFML/System/Clock.hpp>

class Controller;

class Player: public Entity
{
	public:
	Player(Controller *controller);
	~Player();
	virtual Vector2d getSize() const;
	virtual void draw(sf::RenderTarget &target) const;
	virtual Vector2d movement(sf::Int64 tm);
	virtual void event_received(EngineEvent *event);

	void setCanonAngle(float angle);
	void rotateCanon(float angleSpeed);
	void move(Vector2d speed);
	void setPosition(Vector2d position);
	void keepShooting(void);

	private:
	Controller *controller;
	static const float missileDelay; /* milliseconds */
	float tank_direction;
	float canon_direction;
	sf::Color color;
	sf::Clock shoot_clock;

	int playerUID;
	static int UID;

/* dynamic info */
	bool is_shooting;
	double canon_rotation;
	Vector2d tank_movement;
	Vector2d tank_goto;
	sf::Sprite &getSprite(const char *name) const;
};
#endif
