#include "wall.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "texture_cache.h"
#include "engine.h"

using namespace sf;

Wall::Wall(double x, double y, double w, double h, const char *texture_name0, Engine *engine):Entity(SHAPE_RECTANGLE, engine) {
	size.x = w;
	size.y = h;
	position.x = x;
	position.y = y;
	texture_name = texture_name0;
}
Wall::~Wall() {}
Vector2d Wall::getSize() const {
	return Vector2d(size.x, size.y);
}

void Wall::draw(sf::RenderTarget &target) const {
	TextureCache *cache = getEngine()->getTextureCache();
	unsigned ID = cache->getTextureID(texture_name.c_str());
	Sprite &sprite = *(cache->getSprite(ID));
	sprite.setPosition(Vector2f(position.x, position.y));
	sprite.setTextureRect(IntRect(0,0,size.x,size.y));
	target.draw(sprite);
#if 0
	RectangleShape r;
	r.setFillColor(Color(0,255,0));
	r.setOutlineColor(Color(0,0,255));
	r.setPosition(Vector2f(position.x, position.y));
	r.setSize(size);
	target.draw(r);
#endif
}
Vector2d Wall::movement(sf::Int64 tm) {
	return Vector2d(0,0);
}
void Wall::event_received(EngineEvent *event) {
}
