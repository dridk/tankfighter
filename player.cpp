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

void Player::setController(Controller *newc) {
	controller = newc;
}
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
	fprintf(stderr, "[player %d deleted]\n", getUID());
	delete controller;
}
Color Player::getColor() {
	return color;
}
void Player::setColor(Color c) {
	color = c;
}
void Player::killedBy(Player *player) {
	if (!getEngine()->getNetwork()->isClient()) teleport();
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
	controller->teleported();
}

int Player::getScore() {return score;}
void Player::setScore(int sc) {score = sc;}
Controller *Player::getController(void) {
	return controller;
}
Player::Player(Controller *controller0, Engine *engine):Entity(SHAPE_CIRCLE, engine),controller(controller0) {
#if 0
	preserve_tank_angle = false;
	adapt_canon_angle   = false;
	tank_rotation = canon_rotation = 0;
	is_shooting = false;
#endif
	score = 0;
	tank_direction = get_random(2*M_PI);
	canon_direction = get_random(2*M_PI);
	color = Color(get_random(128)+127, get_random(128)+127, get_random(128)+127);
	teleporting = true;
	computeRandomPosition();
	if (controller) controller->setPlayer(this);
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
	if (shoot_clock.getElapsedTime().asMicroseconds() >= ((Int64)missileDelay)*1000) {
		Missile *ml = new Missile(this);
		shoot_clock.restart();
		if (controller->missileCreation(ml)) {
			getEngine()->add(ml);
		} else {
			delete ml;
		}
	}
}

PlayerControllingData::PlayerControllingData() {
	flags = 0;
	canon_rotation = tank_rotation = 0;
	position = movement = Vector2d(0,0);
	new_score = 0;
}
void PlayerControllingData::setCanonAngle(float angle) {
	flags |= PCD_Canon_Angle;
	canon_rotation = 0;
	canon_angle = angle;
}
void PlayerControllingData::rotateCanon(float angleSpeed) {
	canon_rotation = angleSpeed;
}

void PlayerControllingData::preserveTankAngle(void) {
	flags|=PCD_Preserve_Tank_Rotation;
}
void PlayerControllingData::adaptCanonAngle(void) {
	flags|=PCD_Adapt_Canon_Angle;
}
void PlayerControllingData::setTankAngle(float angle) {
	flags |= PCD_Tank_Angle;
	tank_rotation = 0;
	tank_angle = angle;
}
void PlayerControllingData::rotateTank(float angleSpeed) {
	tank_rotation = angleSpeed;
}

void PlayerControllingData::move(Vector2d speed) {
	movement = speed;
}
void PlayerControllingData::setPosition(Vector2d position0) {
	flags |= PCD_Position;
	movement.x = 0;
	movement.y = 0;
	position = position0;
}

void PlayerControllingData::keepShooting(void) {
	flags |= PCD_Shoot;
}
void PlayerControllingData::setScore(int sc) {
	flags |= PCD_Score;
	new_score = sc;
}
void Player::applyPCD(const PlayerControllingData &pcd) {
	if (pcd.flags & PCD_Tank_Angle)  tank_direction  = pcd.tank_angle;
	if (pcd.flags & PCD_Canon_Angle) canon_direction = pcd.canon_angle;

	normalizeAngle(canon_direction);
	normalizeAngle(tank_direction);
	
	if (pcd.flags & PCD_Position) {
		position = pcd.position;
	}
	if (pcd.flags & PCD_Adapt_Canon_Angle) {
		canon_direction = tank_direction;
	}
	if (pcd.flags & PCD_Score) {
		setScore(pcd.new_score);
	}
}
Vector2d Player::movement(Int64 tm) {
	PlayerControllingData pcd;
	Vector2d tank_movement;
	tank_movement.x = tank_movement.y = 0;
	teleporting = false;
#if 0

	is_shooting = false;
	tank_rotation = canon_rotation = 0;
	tank_movement = Vector2d(0,0);
	tank_goto = Vector2d(-1,-1);
	preserve_tank_angle = false;
	adapt_canon_angle   = false;
#endif

	controller->reportPlayerMovement(this, pcd);
	if (pcd.flags & PCD_Shoot) try_shoot();

	if (pcd.flags & PCD_Tank_Angle)  tank_direction  = pcd.tank_angle;
	if (pcd.flags & PCD_Canon_Angle) canon_direction = pcd.canon_angle;

	canon_direction += pcd.canon_rotation * tm * canon_rotation_speed;
	tank_direction  += pcd.tank_rotation  * tm * tank_rotation_speed;

	normalizeAngle(canon_direction);
	normalizeAngle(tank_direction);
	
	if (pcd.flags & PCD_Position) {
		tank_movement = pcd.position - position;
	}
	if (pcd.movement.x != 0 || pcd.movement.y != 0) {
		tank_movement
		+=Vector2d(pcd.movement.x * tm * linear_speed,
			   pcd.movement.y * tm * linear_speed);
	}
	if ((tank_movement.x != 0 || tank_movement.y != 0) && !(pcd.flags & PCD_Preserve_Tank_Rotation)) {
		double angle = angle_from_dxdy(tank_movement.x, tank_movement.y);
		if (angle >= 0 && angle <= 2*M_PI) tank_direction = angle;
	}
	if (pcd.flags & PCD_Adapt_Canon_Angle) {
		canon_direction = tank_direction;
	}
	if (pcd.flags & PCD_Score) {
		setScore(pcd.new_score);
	}
	return tank_movement;
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
}
Sprite &Player::getSprite(const char *name) const {
	return *getEngine()->getTextureCache()->getSprite(name);
}
#if 0
void Player::preserveTankAngle(void) {
	preserve_tank_angle = true;
}
void Player::adaptCanonAngle(void) {
	adapt_canon_angle = true;
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

#endif
