#include "player.h"
#include "misc.h"
#include "engine.h"
#include <SFML/Graphics.hpp>
#include <math.h>
#include "controller.h"
#include "geometry.h"
#include "engine_event.h"
#include "wall.h"
#include <stdio.h>
#include "missile.h"

using namespace sf;

const float Player::missileDelay = 200;
int Player::UID = 0;

Vector2d Player::getPosition() {
	return position;
}
double Player::getCanonAngle() {
	return canon_direction;
}
double Player::getTankAngle() {
	return tank_direction;
}
Player::~Player() {
	delete controller;
}
Color Player::getColor() {
	return color;
}
void Player::killedBy(Player *player) {
	teleport();
}
void Player::killedPlayer(Player *player) {
	setScore(getScore()+1);
}
void Player::computeRandomPosition() {
	Vector2d map_size;
	map_size.x = 800; map_size.y = 600;
	if (getEngine()) map_size = getEngine()->map_size();

	Vector2d plsize = getSize();
	plsize.x += 2;plsize.y += 2;
	position.x = plsize.x + get_random(map_size.x-2*plsize.x);
	position.y = plsize.y + get_random(map_size.y-2*plsize.y);
}
void Player::teleport() {
	teleporting = true;
	computeRandomPosition();
	getEngine()->seekCollisions(this);
	teleporting = false;
}

int Player::getScore() {return score;}
void Player::setScore(int sc) {score = sc;}
Controller *Player::getController(void) {
	return controller;
}
Player::Player(Controller *controller0, Engine *engine):Entity(SHAPE_CIRCLE, engine),controller(controller0) {
	preserve_tank_angle = false;
	adapt_canon_angle   = false;
	tank_rotation = canon_rotation = 0;
	missileCount = 0;
	score = 0;
	is_shooting = false;
	tank_direction = get_random(2*M_PI);
	canon_direction = get_random(2*M_PI);
	color = Color(get_random(128)+127, get_random(128)+127, get_random(128)+127);
	playerUID = ++UID;
	teleporting = true;
	computeRandomPosition();
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
	body.setColor(color);
	target.draw(body);

	Sprite &canon = getSprite("canon");
	r = canon.getLocalBounds();
	canon.setPosition(Vector2f(position.x, position.y));
	canon.setOrigin(Vector2f(r.width/2, r.height/2));
	canon.setRotation(360.0/(2*M_PI)*canon_direction+90);
	canon.setColor(color);
	target.draw(canon);
}
static const double canon_rotation_speed = 3e-4/180*M_PI; /* radians per microsecond */
static const double tank_rotation_speed = 3e-4/180*M_PI;
static const double linear_speed = 3e-4; /* pixels per microsecond */

void Player::try_shoot() {
	if (is_shooting && shoot_clock.getElapsedTime().asMicroseconds() >= ((Int64)missileDelay)*1000 && missileCount < maxMissileCount) {
		shoot_clock.restart();
		getEngine()->add(new Missile(this));
		missileCount++;
	}
}

Vector2d Player::movement(Int64 tm) {
	teleporting = false;

	is_shooting = false;
	tank_rotation = canon_rotation = 0;
	tank_movement = Vector2d(0,0);
	tank_goto = Vector2d(-1,-1);
	preserve_tank_angle = false;
	adapt_canon_angle   = false;

	controller->detectMovement(this);
	try_shoot();

	canon_direction += canon_rotation * tm * canon_rotation_speed;
	tank_direction  += tank_rotation  * tm * tank_rotation_speed;
	normalizeAngle(canon_direction);
	normalizeAngle(tank_direction);
	
	if (tank_goto.x >= -0.5 && tank_goto.y >= -0.5) {
		tank_movement = tank_goto - position;
	} else if (tank_movement.x != 0 || tank_movement.y != 0) {
		tank_movement
		=Vector2d(tank_movement.x * tm * linear_speed,
			tank_movement.y * tm * linear_speed);
	}
	if ((tank_movement.x != 0 || tank_movement.y != 0) && !preserve_tank_angle) {
		double angle = angle_from_dxdy(tank_movement.x, tank_movement.y);
		if (angle >= 0 && angle <= 2*M_PI) tank_direction = angle;
	}
	if (adapt_canon_angle) {
		canon_direction = tank_direction;
	}
	return tank_movement;
}
void Player::preserveTankAngle(void) {
	preserve_tank_angle = true;
}
void Player::adaptCanonAngle(void) {
	adapt_canon_angle = true;
}
void Player::event_received(EngineEvent *event) {
	CompletedMovementEvent *e = dynamic_cast<CompletedMovementEvent*>(event);
	CollisionEvent *coll;
	if (e && e->entity == static_cast<Entity*>(this)) {
		position = e->position;
		return;
	}
	coll = dynamic_cast<CollisionEvent*>(event);
	if (coll && coll->first == static_cast<Entity*>(this)) {
		if (teleporting) { /* teleport it as it has spawned in a another entity */
			computeRandomPosition();
			coll->retry = true;
		}
		if (dynamic_cast<Wall*>(coll->second)) {
			coll->interaction = IT_SLIDE;
		}
		return;
	}
	if (EntityDestroyedEvent *ede = dynamic_cast<EntityDestroyedEvent*>(event)) {
		Missile *missile = dynamic_cast<Missile*>(ede->entity);
		if (missile && missile->getOwner() == this) {
			missileCount--;
		}
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
	normalizeAngle(canon_direction);
	canon_rotation = 0;
}
void Player::setTankAngle(float angle) {
	tank_direction = angle;
	normalizeAngle(tank_direction);
	tank_rotation = 0;
}
void Player::rotateCanon(float angleSpeed) {
	canon_rotation = angleSpeed;
}
void Player::rotateTank(float angleSpeed) {
	tank_rotation = angleSpeed;
}
void Player::move(Vector2d speed) {
	tank_movement += speed;
	tank_goto = Vector2d(-1,-1);
}
void Player::setPosition(Vector2d position) {
	tank_movement = Vector2d(0,0);
	tank_goto = position;
}

