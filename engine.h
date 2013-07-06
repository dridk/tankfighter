#ifndef __ENGINE_H__
#define __ENGINE_H__
#include <deque>
#include "coretypes.h"
#include <SFML/Graphics/RenderWindow.hpp>

class Entity;
class EngineEvent;

class Engine
{
	typedef std::deque<Entity*> Entities;
	typedef Entities::iterator EntitiesIterator;
	public:
	Engine();
	~Engine();
	void add(Entity *entity); /* Must be previously allocated with new. Ownership taken by Engine */
	void destroy(Entity *entity); /* Removes entity from engine and deletes the underlying object */
	void broadcast(EngineEvent *event);
	void quit(void);
	/* Actually, it only set a flag. Entities are really destroyed at the end of the physical frame */

	bool step(void);

	private:
	void draw(void);
	void compute_physics(void);
	void destroy_flagged(void);

	sf::RenderWindow window;
	Entities entities;
	sf::Clock clock;
	bool first_step;
	bool must_quit;
};
#endif
