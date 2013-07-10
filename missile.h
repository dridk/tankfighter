#ifndef __MISSILE_H__
#define __MISSILE_H__
#include "coretypes.h"
#include "entity.h"
#include <SFML/System/Clock.hpp>

class Player;

enum MCDFlags {MCD_Movement = 1, MCD_Position = 2, MCD_Angle = 4};
struct MissileControllingData
{
	unsigned char flags;
	Vector2d movement;
	Vector2d assigned_position;
	double new_angle;
	bool must_die;
	MissileControllingData();
};
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

	double getAngle(void) const;
#if 0
	double setAngle(double angle);
	double setPosition(const Vector2d &pos);
	void move(const Vector2d &vect);
	void Die(void);
	Vector2d movement;
#endif
	sf::Int64 usecGetLifetime(void);
	static const float maxLifeDuration = 2000; /* milliseconds */

	private:
	static const float speed = 9e-4;
	sf::Clock lifetime;
	double angle;
	Player *player;
};
#endif
