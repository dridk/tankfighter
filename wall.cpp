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


Wall::Wall(double x, double y, double w, double h, double angle0, const TextureDesc &texture0, Engine *engine)
		:Entity(SHAPE_POLYGON, engine) {
	DoubleRect r;
	TFPolygon poly;
	r.left = x; r.top = y;
	r.width = w; r.height = h;
	Rectangle2Polygon(r, poly);
	ConstructWall(poly, angle0, texture0);
}
Wall::Wall(const TFPolygon &polygon0, double angle0, const TextureDesc &texture0, Engine *engine)
		:Entity(SHAPE_POLYGON, engine) {
	ConstructWall(polygon0, angle0, texture0);
}
void Wall::ConstructWall(const TFPolygon &polygon0, double angle0, const TextureDesc &texture0) {
	is_map_boundaries = false;
	angle = angle0;
	straight_polygon = polygon0;
	polygon = polygon0;
	RotatePolygon(polygon, angle0);
	ComputePosition();
	texture = texture0;
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
TFPolygon Wall::getPolygon(void) const {
	return polygon;
}
void Wall::draw(sf::RenderTarget &target) const {
	TextureCache *cache = getEngine()->getTextureCache();
	unsigned ID = cache->getTextureID(texture.name.c_str());
	Vector2f corescale = cache->getSprite(ID)->getScale();
	Vector2f texscale = corescale;
	Texture *sftexture = cache->getTexture(ID);
	Vector2u texsz = sftexture->getSize();
	
	TFPolygon rpoly = getStraightPolygon();
	DoubleRect r0 = getPolyBounds(rpoly);
	
	if (texture.mapping == MAPPING_TILE_ABSOLUTE) RotatePolygon(rpoly, angle-texture.angle);
	else RotatePolygon(rpoly, -texture.angle);
	
	DoubleRect r = getPolyBounds(rpoly);
	ConvexShape shape(polygon.size());
	if (texture.mapping != MAPPING_TILE_ABSOLUTE) {
		shape.setRotation(180/M_PI*(angle + texture.angle));
		shape.setOrigin(polygon[0].x, polygon[0].y);
		shape.setPosition(polygon[0].x, polygon[0].y);
	} else {
		shape.setRotation(180/M_PI*texture.angle);
		shape.setOrigin(polygon[0].x, polygon[0].y);
		shape.setPosition(polygon[0].x, polygon[0].y);
	}
	shape.setTexture(sftexture);
	sftexture->setRepeated(true);
	
	if (texture.mapping == MAPPING_STRETCH) {
		texscale = Vector2f(r0.width/texsz.x * texture.xscale, r0.height/texsz.y  * texture.yscale);
	} else {
		texscale = Vector2f(corescale.x * texture.xscale, corescale.y * texture.yscale);
	}
	double xoff = 0, yoff = 0;
	if (texture.mapping != MAPPING_TILE_ABSOLUTE) {
		xoff += r.left - r0.left;
		yoff += r.top - r0.top;
	}
	if (texture.mapping == MAPPING_TILE_ABSOLUTE) {
		/* my requirement is about polygon[0] point alignment... */
		/* I can compute where it should be on the texture */
		/* And then, compute where the r.top/r.left should be on the texture...*/
		Vector2f p0t(polygon[0].x, polygon[0].y);
		Transform rottex;
		rottex.rotate(-180/M_PI*texture.angle);
		p0t = rottex.transformPoint(p0t);
		
		xoff += p0t.x + (r.left - polygon[0].x);
		yoff += p0t.y + (r.top - polygon[0].y);
	}
	
	shape.setTextureRect(IntRect(xoff/texscale.x + texture.xoff*corescale.x, yoff/texscale.y + texture.yoff*corescale.y, r.width/texscale.x, r.height/texscale.y));
	shape.setFillColor(texture.color);
	
	for(size_t i=0; i < rpoly.size(); i++) {
		Vector2f pt(rpoly[i].x, rpoly[i].y);
		shape.setPoint(i, pt);
	}
	target.draw(shape);
	sftexture->setRepeated(false);
}
Vector2d Wall::movement(sf::Int64 tm) {
	return Vector2d(0,0);
}
void Wall::event_received(EngineEvent *event) {
}
std::string Wall::getTextureName(void) const {
	return texture.name;
}
void Wall::getPolygon(TFPolygon &opoly) {
	opoly = polygon;
}
double Wall::getAngle() const {return angle;}
double Wall::getTextureAngle() const {return texture.angle;}

void TextureDesc::clear() {
	name = "";
	mapping = MAPPING_TILE;
	color   = Color(255,255,255,255);
	xscale = yscale = 1;
	xoff = yoff = 0;
	angle = 0;
}
TextureDesc::TextureDesc() {
	clear();
}
TextureDesc::TextureDesc(const char *texture_name0) {
	clear();
	name=(texture_name0?texture_name0:"");
}
bool Wall::isMapBoundaries() const {
	return is_map_boundaries;
}
void Wall::isMapBoundaries(bool v) {
	is_map_boundaries = v;
}
Color Wall::getColor(void) {
	return texture.color;
}
Mapping Wall::getMappingType() {
	return texture.mapping;
}
TextureScales Wall::getTextureScales(void) const {
	TextureScales t;
	t.xscale = texture.xscale;
	t.yscale = texture.yscale;
	t.xoff = texture.xoff;
	t.yoff = texture.yoff;
	return t;
}
