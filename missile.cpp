#include "missile.h"
#include "player.h"
#include "wall.h"
#include "coretypes.h"
#include "engine.h"
#include "engine_event.h"
#include "geometry.h"
#include <math.h>
#include <stdio.h>

using namespace sf;

Missile::Missile(Player *player0)
	:Entity(SHAPE_CIRCLE, player0->getEngine()),player(player0) {
	position = player->position;
	angle = player->getCanonAngle();
}
Missile::~Missile() {
	player = NULL;
}
Vector2d Missile::getSize() const {
	Vector2d sz;
	sz.x = 16;
	sz.y = 16;
	return sz;
}
Player *Missile::getOwner(void) const {
	return player;
}
void Missile::draw(sf::RenderTarget &target) const {
	Sprite &sprite = *getEngine()->getTextureCache()->getSprite("bullet");
	FloatRect r = sprite.getLocalBounds();
	sprite.setOrigin(Vector2f(r.width/2, r.height/2));

	sprite.setPosition(Vector2f(position.x, position.y));
	sprite.setRotation(360/(2*M_PI)*angle+90);
	if (player) {
		sprite.setColor(player->getColor());
	}
	target.draw(sprite);
}
Vector2d Missile::movement(Int64 tm) {
	Vector2d v;
	v.x = cos(angle) * tm * speed;
	v.y = sin(angle) * tm * speed;
	if (lifetime.getElapsedTime().asMicroseconds() >= 1000*(Int64)maxLifeDuration) {
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
			if (ce->second == getEngine()->getMapBoundariesEntity()) {
				getEngine()->destroy(this);
			}
			if (dynamic_cast<Wall*>(ce->second)) {
				ce->interaction = IT_BOUNCE;
			} else if (static_cast<Entity*>(player) != ce->second && (dying = dynamic_cast<Player*>(ce->second))) {
				dying->killedBy(player);
				player->killedPlayer(dying);
				/* missile dies */
				ce->interaction = IT_GHOST;
				getEngine()->destroy(this);
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

