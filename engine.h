#ifndef __ENGINE_H__
#define __ENGINE_H__
#include <deque>
#include "coretypes.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "texture_cache.h"

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
	Entity *getMapBoundariesEntity(void);
	/* Actually, it only set a flag. Entities are really destroyed at the end of the physical frame */

	bool step(void);
	TextureCache *getTextureCache(void) const;
	sf::RenderWindow &getWindow(void) {return window;}
	const sf::RenderWindow &getWindow(void) const {return window;}
	Vector2d map_size(void);

	private:
	void draw(void);
	void compute_physics(void);
	void destroy_flagged(void);
	void seekCollisions(Entity *entity);

	mutable TextureCache texture_cache;

	Entity *map_boundaries_entity;
	sf::Texture background_texture;
	sf::Sprite background;
	sf::RenderWindow window;
	Entities entities;
	sf::Clock clock;
	bool first_step;
	bool must_quit;
};
#endif
