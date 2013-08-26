#ifndef __WALL_H__
#define __WALL_H__
#include "entity.h"
#include <string>
#include "polygon.h"
#include "coretypes.h"
#include <string>

enum Mapping {MAPPING_TILE, MAPPING_STRETCH, MAPPING_TILE_ABSOLUTE};
struct TextureDesc {
	std::string name;
	sf::Color color;
	float xscale, yscale, xoff, yoff;
	float angle;
	Mapping mapping;
	TextureDesc();
	TextureDesc(const char *texture_name);
	void clear();
};

class Engine;

class Wall: public Entity
{
	public:
	Wall(double x, double y, double w, double h, double angle, const TextureDesc &texture, Engine *engine);
	Wall(const TFPolygon &polygon, double angle, const TextureDesc &texture, Engine *engine);
	virtual ~Wall();
	virtual Vector2d getSize() const;
	virtual void getPolygon(TFPolygon &poly);
	TFPolygon getPolygon(void) const;
	TFPolygon getStraightPolygon(void) const;

	double getTextureAngle() const;
	double getAngle() const;

	virtual void draw(sf::RenderTarget &target) const;
	virtual Vector2d movement(sf::Int64 tm);
	virtual void event_received(EngineEvent *event);
	std::string getTextureName(void) const;
	bool isMapBoundaries() const;
	void isMapBoundaries(bool v);

	private:
	void ConstructWall(const TFPolygon &polygon, double angle, const TextureDesc &texture);
	DoubleRect getBoundingRectangle() const;
	void ComputePosition();
	TFPolygon polygon, straight_polygon;
	TextureDesc texture;
	float angle;
	bool is_map_boundaries;
};

DoubleRect getPolyBounds(const TFPolygon &polygon);
#endif
