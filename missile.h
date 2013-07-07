#ifndef __MISSILE_H__
#define __MISSILE_H__
#include "coretypes.h"
#include "entity.h"

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

	private:
	static const float speed = 9e-4;
	double angle;
	Player *player;
};
#endif
