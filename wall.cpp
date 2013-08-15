#include "wall.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "texture_cache.h"
#include "engine.h"
#include <math.h>

using namespace sf;

Wall::Wall(double x, double y, double w, double h, double angl, const char *texture_name0, Engine *engine)
		:Entity(SHAPE_RECTANGLE, engine),angle(angl) {
	size.x = w;
	size.y = h;
	position.x = x;
	position.y = y;
	texture_name = (texture_name0?texture_name0:"");
}
Wall::Wall(double x, double y, double w, double h, const char *texture_name0, Engine *engine)
		:Entity(SHAPE_RECTANGLE, engine),angle(0) {
	size.x = w;
	size.y = h;
	position.x = x;
	position.y = y;
	texture_name = (texture_name0?texture_name0:"");
}
Wall::~Wall() {}
Vector2d Wall::getSize() const {
	return Vector2d(size.x, size.y);
}
double Wall::getAngle() const {
	return angle;
}

void Wall::draw(sf::RenderTarget &target) const {
	if (texture_name == "") return;
	TextureCache *cache = getEngine()->getTextureCache();
	unsigned ID = cache->getTextureID(texture_name.c_str());
	Sprite &sprite = *(cache->getSprite(ID));
	sprite.setPosition(Vector2f(position.x, position.y));
	sprite.setTextureRect(IntRect(0,0,size.x,size.y));
	sprite.setRotation(180/M_PI*angle);
	target.draw(sprite);
}
Vector2d Wall::movement(sf::Int64 tm) {
	return Vector2d(0,0);
}
void Wall::event_received(EngineEvent *event) {
}
std::string Wall::getTextureName(void) const {
	return texture_name;
}
