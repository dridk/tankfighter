#include "controller.h"
#include "player.h"
#include "geometry.h"
#include "engine.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <math.h>
#include "input.h"

using namespace sf;

static void detectKeyboardMovement(Player *player, PlayerControllingData &pcd) {
	int vertical = 0;
	int horiz    = 0;
	if (isLocalKeyPressed(Keyboard::Up) && !isLocalKeyPressed(Keyboard::Down)) {
		vertical = -1;
	} else if (isLocalKeyPressed(Keyboard::Down) && !isLocalKeyPressed(Keyboard::Up)) {
		vertical = 1;
	}
	if (isLocalKeyPressed(Keyboard::Left) && !isLocalKeyPressed(Keyboard::Right)) {
		horiz = -1;
	} else if (isLocalKeyPressed(Keyboard::Right) && !isLocalKeyPressed(Keyboard::Left)) {
		horiz = 1;
	}
	Vector2d v;
	v.y = vertical;
	v.x = horiz;
	if (horiz != 0 || vertical !=0) {
		normalizeVector(v, 1);
		pcd.move(v);
	}
	if (isLocalKeyPressed(Keyboard::Escape)) {
		player->getEngine()->quit();
	}
}
static void detectMouseMovement(Player *player, PlayerControllingData &pcd) {
	double dx, dy;
	double angle = -1;
	Vector2d pos = player->getEngine()->getMousePosition();
	dy = pos.y - player->position.y;
	dx = pos.x - player->position.x;
	if (dx != 0 && dy != 0) angle = angle_from_dxdy(dx, dy);
	if (angle >= 0 && angle <= 2*M_PI) {
		pcd.setCanonAngle(angle);
	}
}
void KeyboardMouseController::KeyboardMouseController::reportPlayerMovement(Player *player, PlayerControllingData &pcd) {
	detectKeyboardMovement(player, pcd);
	detectMouseMovement(player, pcd);
	if (Mouse::isButtonPressed(Mouse::Left) || isLocalKeyPressed(Keyboard::RControl) || isLocalKeyPressed(Keyboard::LControl)) {
		pcd.keepShooting();
	}
}
