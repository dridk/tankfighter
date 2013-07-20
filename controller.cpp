#include "controller.h"
#include "missile.h"
#include <math.h>
#include "coretypes.h"
#include "engine.h"
#include "parameters.h"

using namespace sf;

Controller::Controller() {
	player = NULL;
}
Controller::~Controller() {}
void Controller::reportMissileMovement(Missile *missile, MissileControllingData &mcd) {
	double angle = missile->getAngle();
	mcd.flags = MCD_Movement;
	mcd.movement = Vector2d(cos(angle), sin(angle));
	mcd.must_die = missile->usecGetLifetime() >= 1000*(Int64)parameters.maxMissileLifeDurationMS();
}
void Controller::setPlayer(Player *player0) {
	player = player0;
}
Player *Controller::getPlayer(void) const {return player;}

bool LocalController::isConcerned(const sf::Event &e) const {
	return false;
}
bool Controller::missileCreation(Missile *ml) {
	return ml->getEngine()->canCreateMissile(ml->getOwner());
}
bool Controller::missileCollision(Missile *, Player *) {
	return true;
}
void Controller::teleported(void) {
}
