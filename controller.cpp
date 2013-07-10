#include "controller.h"
#include "missile.h"
#include <math.h>
#include "coretypes.h"

using namespace sf;

Controller::~Controller() {}
void Controller::reportMissileMovement(Missile *missile, MissileControllingData &mcd) {
	double angle = missile->getAngle();
	mcd.flags = MCD_Movement;
	mcd.movement = Vector2d(cos(angle), sin(angle));
	mcd.must_die = missile->usecGetLifetime() >= 1000*(Int64)Missile::maxLifeDuration;
}
