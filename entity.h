#ifndef __ENTITY__H
#define __ENTITY__H
#include <SFML/Graphics.h>

class EngineEvent;
class Engine;

class Entity
{
	public:
	enum Shape {CIRCLE_SHAPE, RECTANGLE_SHAPE};
	Shape shape;
	Vector2f position;

	FloatRect getBoundingRect(void) const;
	Entity();
	virtual ~Entity();
	virtual Vector2f getSize() const;

	/* called by graphics engine */
	virtual void draw(sf::RenderTarget target) const = 0;
	/* call by physics engine. This function returns the relative movement that SHOULD happen if no obstacle is found */
	virtual Vector2f movement(Int64 tm) = 0;
	/* then, the physics engine call the HIT functions if an obstacle is in the way of the movement */
	virtual void event_received(EngineEvent *event) = 0;

	void setKilled(void) {isKilled=true;}
	bool isKilled(void) {return isKilled;}
	void setEngine(Engine *eng) {engine = eng;}
	Engine *getEngine(void) const {return engine;}
	private:
	bool isKilled;
	Engine *engine;
};

#endif
