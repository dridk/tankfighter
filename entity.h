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

	Entity(EntityShape shape, Engine *engine);
	virtual ~Entity();
	virtual Vector2d getSize() const = 0;

	/* called by graphics engine */
	virtual void draw(sf::RenderTarget &target) const = 0;
	/* call by physics engine. This function returns the relative movement that SHOULD happen if no obstacle is found */
	virtual Vector2d movement(sf::Int64 tm) = 0;
	/* then, the physics engine call the HIT functions if an obstacle is in the way of the movement */
	virtual void event_received(EngineEvent *event) = 0;

	void setKilled(bool flag=true) {isKilledFlag=flag;}
	bool isKilled(void) {return isKilledFlag;}
	void setEngine(Engine *eng) {engine = eng;}
	Engine *getEngine(void) const {return engine;}
	sf::Uint32 getUID(void) const;
	void setUID(sf::Uint32 uid);
	static void useUpperUID(void);
	private:
	static sf::Uint32 globalUID;
	sf::Uint32 UID;
	bool isKilledFlag;
	Engine *engine;
};

#endif
