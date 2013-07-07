#include "player.h"
#include "misc.h"
#include "engine.h"
#include <SFML/Graphics.hpp>
#include <math.h>
#include "controller.h"
#include "geometry.h"
#include "engine_event.h"
#include <stdio.h>

using namespace sf;

const float Player::missileDelay = 200;
int Player::UID = 0;

Player::~Player() {
	delete controller;
}
Player::Player(Controller *controller0):Entity(SHAPE_CIRCLE),controller(controller0) {
	is_shooting = false;
	position.x = get_random(800);
	position.y = get_random(600);
	tank_direction = get_random(2*M_PI);
	canon_direction = get_random(2*M_PI);
	color = Color(get_random(255), get_random(255), get_random(255));
	playerUID = ++UID;
}

Vector2d Player::getSize() const {
	Vector2d sz;
	sz.x = 128;
	sz.y = 128;
	return sz;
}
void Player::draw(sf::RenderTarget &target) const {
	Sprite &body = getSprite("car");
	FloatRect r = body.getLocalBounds();
	body.setPosition(Vector2f(position.x, position.y));
	body.setOrigin(Vector2f(r.width/2, r.height/2));
	body.setRotation(360.0/(2*M_PI)*tank_direction+90);
	target.draw(body);

	Sprite &tank = getSprite("canon");
	r = tank.getLocalBounds();
	tank.setPosition(Vector2f(position.x, position.y));
	tank.setOrigin(Vector2f(r.width/2, r.height/2));
	tank.setRotation(360.0/(2*M_PI)*canon_direction+90);
	target.draw(tank);
}
static const double canon_rotation_speed = 3e-4/180*M_PI; /* radians per microsecond */
static const double linear_speed = 3e-4; /* pixels per microsecond */
Vector2d Player::movement(Int64 tm) {
	is_shooting = false;
	canon_rotation = 0;
	tank_movement = Vector2d(0,0);
	tank_goto = Vector2d(-1,-1);

	controller->detectMovement(this);

	canon_direction += canon_rotation * tm * canon_rotation_speed;
	
	if (tank_goto.x >= -0.5 && tank_goto.y >= -0.5) {
		tank_movement = tank_goto - position;
	} else if (tank_movement.x != 0 || tank_movement.y != 0) {
		tank_movement
		=Vector2d(tank_movement.x * tm * linear_speed,
			tank_movement.y * tm * linear_speed);
	}
	if (tank_movement.x != 0 || tank_movement.y != 0) {
		double angle = angle_from_dxdy(tank_movement.x, tank_movement.y);
		if (angle >= 0 && angle <= 2*M_PI) tank_direction = angle;
	}
	fprintf(stderr, "[move rel %lg %lg]\n"
		, tank_movement.x * tm * linear_speed
		, tank_movement.y * tm * linear_speed);
	return tank_movement;
}
void Player::event_received(EngineEvent *event) {
	CompletedMovementEvent *e = dynamic_cast<CompletedMovementEvent*>(event);
	if (e && e->entity == this) {
		fprintf(stderr, "[completed %lg %lg]\n", e->position.x, e->position.y);
		position = e->position;
	}
}
Sprite &Player::getSprite(const char *name) const {
	return *getEngine()->getTextureCache()->getSprite(name);
}

void Player::keepShooting(void) {
	is_shooting = true;
}
void Player::setCanonAngle(float angle) {
	canon_direction = angle;
	canon_rotation = 0;
}
void Player::rotateCanon(float angleSpeed) {
	canon_rotation += angleSpeed;
	normalizeAngle(canon_rotation);
}
void Player::move(Vector2d speed) {
	tank_movement += speed;
	tank_goto = Vector2d(-1,-1);
}
void Player::setPosition(Vector2d position) {
	tank_movement = Vector2d(0,0);
	tank_goto = position;
}

