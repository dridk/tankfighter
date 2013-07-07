#include "controller.h"
#include "player.h"
#include "geometry.h"
#include "engine.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <math.h>

using namespace sf;

void detectKeyboardMovement(Player *player) {
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
		player->move(v);
	}
	if (Keyboard::isKeyPressed(Keyboard::Escape)) {
		player->getEngine()->quit();
	}
}
void detectMouseMovement(Player *player) {
	double dx, dy;
	double angle;
	RenderWindow &window = player->getEngine()->getWindow();
	Vector2i pos = Mouse::getPosition(window);
	dy = pos.y - player->position.y;
	dx = pos.x - player->position.x;
	if (dx != 0 && dy != 0) angle = angle_from_dxdy(dx, dy);
	if (angle >= 0 && angle <= 2*M_PI) {
		player->setCanonAngle(angle);
	}
}
void KeyboardMouseController::KeyboardMouseController::detectMovement(Player *player) {
	detectKeyboardMovement(player);
	detectMouseMovement(player);
	if (Mouse::isButtonPressed(Mouse::Left) || Keyboard::isKeyPressed(Keyboard::RControl) || Keyboard::isKeyPressed(Keyboard::LControl)) {
		player->keepShooting();
	}
}
