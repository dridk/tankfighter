#ifndef __MISSILE_H__
#define __MISSILE_H__
#include "coretypes.h"
#include "entity.h"

class Missile: public Entity
{
	virtual Vector2d getSize() const;
	virtual void draw(sf::RenderTarget target) const;
	virtual Vector2d movement(Int64 tm);
	virtual void event_received(EngineEvent *event);

	private:
	static const float speed = 3;
	double x, y, angle;
	int playerUID;
};
#endif
