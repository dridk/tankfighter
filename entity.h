#ifndef __ENTITY__H
#define __ENTITY__H
#include "coretypes.h"
#include <SFML/Graphics/RenderTarget.hpp>

class EngineEvent;
class Engine;

enum EntityShape {SHAPE_CIRCLE, SHAPE_RECTANGLE};
class Entity
{
	public:
	EntityShape shape;
	Vector2d position;

	DoubleRect getBoundingRect(void) const;
	Entity();
	virtual ~Entity();
	virtual Vector2d getSize() const;

	/* called by graphics engine */
	virtual void draw(sf::RenderTarget &target) const = 0;
	/* call by physics engine. This function returns the relative movement that SHOULD happen if no obstacle is found */
	virtual Vector2d movement(sf::Int64 tm) = 0;
	/* then, the physics engine call the HIT functions if an obstacle is in the way of the movement */
	virtual void event_received(EngineEvent *event) = 0;

	void setKilled(void) {isKilledFlag=true;}
	bool isKilled(void) {return isKilledFlag;}
	void setEngine(Engine *eng) {engine = eng;}
	Engine *getEngine(void) const {return engine;}
	private:
	bool isKilledFlag;
	Engine *engine;
};

#endif
