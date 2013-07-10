#include "controller.h"
#include "player.h"
#include "geometry.h"
#include "engine.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <math.h>

using namespace sf;

static void detectKeyboardMovement(Player *player, PlayerControllingData &pcd) {
	int vertical = 0;
	int horiz    = 0;
	if (Keyboard::isKeyPressed(Keyboard::Up) && !Keyboard::isKeyPressed(Keyboard::Down)) {
		vertical = -1;
	} else if (Keyboard::isKeyPressed(Keyboard::Down) && !Keyboard::isKeyPressed(Keyboard::Up)) {
		vertical = 1;
	}
	if (Keyboard::isKeyPressed(Keyboard::Left) && !Keyboard::isKeyPressed(Keyboard::Right)) {
		horiz = -1;
	} else if (Keyboard::isKeyPressed(Keyboard::Right) && !Keyboard::isKeyPressed(Keyboard::Left)) {
		horiz = 1;
	}
	Vector2d v;
	v.y = vertical;
	v.x = horiz;
	if (horiz != 0 || vertical !=0) {
		normalizeVector(v, 1);
		pcd.move(v);
	}
	if (Keyboard::isKeyPressed(Keyboard::Escape)) {
		player->getEngine()->quit();
	}
}
static void detectMouseMovement(Player *player, PlayerControllingData &pcd) {
	double dx, dy;
	double angle = -1;
	RenderWindow &window = player->getEngine()->getWindow();
	Vector2i pos = Mouse::getPosition(window);
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
	if (Mouse::isButtonPressed(Mouse::Left) || Keyboard::isKeyPressed(Keyboard::RControl) || Keyboard::isKeyPressed(Keyboard::LControl)) {
		pcd.keepShooting();
	}
}
