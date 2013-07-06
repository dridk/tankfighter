#ifndef __PLAYER_H__
#define __PLAYER_H__
#include "coretypes.h"
#include "entity.h"
#include <SFML/System.hpp>

class Player: public Entity
{
	public:
	virtual Vector2d getSize() const;
	virtual void draw(sf::RenderTarget target) const;
	virtual Vector2d movement(Int64 tm);
	virtual void event_received(EngineEvent *event);

	void setCanonAngle(float angle);
	void rotateCanon(float angleSpeed);
	void move(Vector2d speed);
	void setPosition(Vector2f position);
	void keepShooting(void);

	private:
	static const missileDelay = 200; /* milliseconds */
	Vector2f tank_position;
	float tank_direction;
	float canon_direction;
	Color color;
	Clock shoot_clock;

	int playerUID;
	static int UID;
};
#endif
