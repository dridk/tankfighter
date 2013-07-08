#ifndef __MISSILE_H__
#define __MISSILE_H__
#include "coretypes.h"
#include "entity.h"
#include <SFML/System/Clock.hpp>

class Player;

class Missile: public Entity
{
	public:
	Missile(Player *player);
	~Missile();
	virtual Vector2d getSize() const;
	virtual void draw(sf::RenderTarget &target) const;
	virtual Vector2d movement(sf::Int64 tm);
	virtual void event_received(EngineEvent *event);
	Player *getOwner() const;

	private:
	static const float speed = 9e-4;
	static const float maxLifeDuration = 2000; /* milliseconds */
	sf::Clock lifetime;
	double angle;
	Player *player;
};
#endif
