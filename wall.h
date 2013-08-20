#ifndef __WALL_H__
#define __WALL_H__
#include "entity.h"
#include <string>
#include "polygon.h"
#include "coretypes.h"

class Engine;

class Wall: public Entity
{
	public:
	Wall(double x, double y, double w, double h, double angle, const char *texture_name, Engine *engine);
	Wall(const TFPolygon &polygon, double angle, const char *texture_name, Engine *engine);
	virtual ~Wall();
	virtual Vector2d getSize() const;
	virtual void getPolygon(TFPolygon &poly);
	TFPolygon getStraightPolygon(void) const;
	virtual double getTextureAngle() const;

	virtual void draw(sf::RenderTarget &target) const;
	virtual Vector2d movement(sf::Int64 tm);
	virtual void event_received(EngineEvent *event);
	std::string getTextureName(void) const;

	private:
	void ConstructWall(const TFPolygon &polygon, double angle, const char *texture_name);
	DoubleRect getBoundingRectangle() const;
	void ComputePosition();
	TFPolygon polygon, straight_polygon;
	std::string texture_name;
	float angle;
};

DoubleRect getPolyBounds(const TFPolygon &polygon);
#endif
