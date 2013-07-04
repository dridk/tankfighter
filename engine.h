#ifndef __ENGINE_H__
#define __ENGINE_H__
#include <deque>

class Entity;
class EngineEvent;

class Engine
{
	typedef std::deque<Entity*> Entities;
	typedef Entities::iterator EntitiesIterator;
	public:
	Engine();
	void add(Entity *entity); /* Must be previously allocated with new. Ownership taken by Engine */
	void destroy(Entity *entity); /* Removes entity from engine and deletes the underlying object */
	void broadcast(EngineEvent *event);
	/* Actually, it only set a flag. Entities are really destroyed at the end of the physical frame */

	bool step(void);

	private:
	void draw(void);
	void compute_physics(void);
	void destroy_flagged(void);

	Entities entities;
	bool must_quit;
};
#endif
