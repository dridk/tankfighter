#ifndef __MISSILE_H__
#define __MISSILE_H__
#include "entity.h"

class Missile: public Entity
{
	virtual Vector2f getSize() const;
	virtual void draw(sf::RenderTarget target) const;
	virtual Vector2f movement(Int64 tm);
	virtual void event_received(EngineEvent *event);

	private:
	static const float speed = 3;
	float x, y, angle;
	int playerUID;
};
#endif
