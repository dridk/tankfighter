#include "wall.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "texture_cache.h"
#include "engine.h"
#include <math.h>
#include "misc.h"
#include "geometry.h"
#include <algorithm>
#include <stdio.h>

using namespace sf;

Wall::Wall(double x, double y, double w, double h, double angle0, const char *texture_name0, Engine *engine)
		:Entity(SHAPE_POLYGON, engine) {
	DoubleRect r;
	TFPolygon poly;
	r.left = x; r.top = y;
	r.width = w; r.height = h;
	Rectangle2Polygon(r, poly);
	ConstructWall(poly, angle0, texture_name0);
}
Wall::Wall(const TFPolygon &polygon0, double angle0, const char *texture_name0, Engine *engine)
		:Entity(SHAPE_POLYGON, engine) {
	ConstructWall(polygon0, angle0, texture_name0);
}
void Wall::ConstructWall(const TFPolygon &polygon0, double angle0, const char *texture_name0) {
	angle = angle0;
	straight_polygon = polygon0;
	polygon = polygon0;
	RotatePolygon(polygon, angle0);
	ComputePosition();
	texture_name = (texture_name0?texture_name0:"");
}
Wall::~Wall() {}
Vector2d Wall::getSize() const {
	DoubleRect r = getBoundingRectangle();
	return Vector2d(r.width, r.height);
}
void Wall::ComputePosition() {
	DoubleRect r = getBoundingRectangle();
	position.x = r.left;
	position.y = r.top;
}
DoubleRect getPolyBounds(const TFPolygon &polygon) {
	Vector2d minp, maxp;
	if (polygon.size() > 0) minp = maxp = polygon[0];
	for(size_t i=0; i < polygon.size(); i++) {
		minp.x = std::min(minp.x, polygon[i].x);
		minp.y = std::min(minp.y, polygon[i].y);
		maxp.x = std::max(maxp.x, polygon[i].x);
		maxp.y = std::max(maxp.y, polygon[i].y);
	}
	return DoubleRect(minp.x, minp.y, maxp.x - minp.x, maxp.y - minp.y);
}
DoubleRect Wall::getBoundingRectangle() const {
	return getPolyBounds(polygon);
}
TFPolygon Wall::getStraightPolygon(void) const {
	return straight_polygon;
}
void Wall::draw(sf::RenderTarget &target) const {
	if (texture_name == "") return;
	TextureCache *cache = getEngine()->getTextureCache();
	unsigned ID = cache->getTextureID(texture_name.c_str());
	DoubleRect r = getPolyBounds(getStraightPolygon());
	ConvexShape shape(polygon.size());
	shape.setPosition(position.x, position.y);
	shape.setRotation(180/M_PI*angle);
	const Transform &itr = shape.getInverseTransform();
	for(size_t i=0; i < polygon.size(); i++) {
		Vector2f pt(polygon[i].x, polygon[i].y);
		shape.setPoint(i, itr.transformPoint(pt));
	}
	shape.setTexture(cache->getTexture(ID));
	shape.setTextureRect(IntRect(0,0,r.width,r.height));
	target.draw(shape);
}
Vector2d Wall::movement(sf::Int64 tm) {
	return Vector2d(0,0);
}
void Wall::event_received(EngineEvent *event) {
}
std::string Wall::getTextureName(void) const {
	return texture_name;
}
void Wall::getPolygon(TFPolygon &opoly) {
	opoly = polygon;
}
double Wall::getTextureAngle() const {return angle;}
