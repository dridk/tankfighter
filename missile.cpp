#include "missile.h"
#include "player.h"
#include "wall.h"
#include "coretypes.h"
#include "engine.h"
#include "engine_event.h"
#include "geometry.h"
#include <math.h>
#include <stdio.h>
#include "controller.h"
#include "parameters.h"
#include "misc.h"

using namespace sf;

Missile::Missile(Player *player0)
	:Entity(SHAPE_CIRCLE, player0->getEngine()),player(player0) {
	Vector2d cvect;
	position = player->position;
	angle = player->getCanonAngle();
	double canon_length = parameters.getCanonLength();
	cvect.x = cos(angle)*canon_length;
	cvect.y = sin(angle)*canon_length;
	position += cvect;
}
Missile::~Missile() {
	player = NULL;
}
Vector2d Missile::getSize() const {
	Vector2d sz;
	sz.x = parameters.missileDiameter();
	sz.y = sz.x;
	return sz;
}
Player *Missile::getOwner(void) const {
	return player;
}
void Missile::draw(sf::RenderTarget &target) const {
	Sprite &sprite = *getEngine()->getTextureCache()->getSprite(parameters.missileSpriteName().c_str());
	FloatRect r = sprite.getLocalBounds();
	sprite.setOrigin(Vector2f(r.width/2, r.height/2));

	sprite.setPosition(Vector2f(position.x, position.y));
	sprite.setRotation(360/(2*M_PI)*angle+90);
	if (player) {
		sprite.setColor(player->getColor());
	}
	drawSprite(sprite, target);
}
double Missile::getAngle(void) const {return angle;}
MissileControllingData::MissileControllingData() {
	flags = 0;
	movement = Vector2d(0,0);
	assigned_position = Vector2d(0, 0);
	new_angle = 0;
	must_die = false;
}
#if 0
double Missile::setAngle(double nangle) {angle = nangle;}
double Missile::setPosition(const Vector2d &pos) {
	position = pos;
	movement.x = 0;
	movement.y = 0;
}
void Missile::move(const Vector2d &vect) {
	movement = vect;
}
void Missile::Die(void) {
	setKilled();
}
#endif
Int64 Missile::usecGetLifetime(void) {
	return lifetime.getElapsedTime().asMicroseconds();
}
Vector2d Missile::movement(Int64 tm) {
	Vector2d v;
	MissileControllingData mcd;

	player->getController()->reportMissileMovement(this, mcd);

	if (mcd.flags & MCD_Position) {
		position = mcd.assigned_position;
		v.x = 0;
		v.y = 0;
	} else if (mcd.flags & MCD_Movement) {
		v.x = mcd.movement.x * tm * parameters.missileSpeed();
		v.y = mcd.movement.y * tm * parameters.missileSpeed();
	}
	if (mcd.flags & MCD_Angle) {
		angle = mcd.new_angle;
	}
	if (mcd.must_die) {
		getEngine()->destroy(this);
	}
	return v;
}
void Missile::event_received(EngineEvent *event) {
	Player *dying;
	if (EntityDestroyedEvent *de = dynamic_cast<EntityDestroyedEvent*>(event)) {
		if (de->entity == static_cast<Entity*>(player)) {
			player = NULL;
			getEngine()->destroy(this);
		}
	} else if (CollisionEvent *ce = dynamic_cast<CollisionEvent*>(event)) {
		if (ce->first == static_cast<Entity*>(this)) {
			if (dynamic_cast<Wall*>(ce->second)) {
				ce->interaction = IT_BOUNCE;
			} else if (static_cast<Entity*>(player) != ce->second && (dying = dynamic_cast<Player*>(ce->second))) {
				ce->interaction = IT_GHOST;
				if (player->getController()->missileCollision(this,dying)) {
					dying->killedBy(player);
					player->killedPlayer(dying);
					/* missile dies */
					getEngine()->destroy(this);
				}
			}
		}
	} else if (CompletedMovementEvent *cme = dynamic_cast<CompletedMovementEvent*>(event)) {
		if (cme->entity == static_cast<Entity*>(this)) {
			position = cme->position;
			if (cme->has_new_speed) {
				double nangle = angle_from_dxdy(cme->new_speed.x, cme->new_speed.y);
				if (fabs(nangle - angle) >= M_PI*1e-4) angle = nangle;
			}
		}
	}
}

void Missile::setAngle(double angle0) {
	angle = angle0;
}
void Missile::setPosition(const Vector2d &position0) {
	position = position0;
}

