#ifndef __ENGINE_H__
#define __ENGINE_H__
#include <deque>
#include "coretypes.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window.hpp>
#include "texture_cache.h"
#include "load_map.h"

class Entity;
class EngineEvent;
class Player;
class Missile;

class Engine
{
	typedef std::deque<Entity*> Entities;
	public:
	typedef Entities::iterator EntitiesIterator;
	EntitiesIterator begin_entities();
	EntitiesIterator end_entities();
	Engine();
	~Engine();
	void addPlayer(unsigned controllerType, int joyid=-1);
	void add(Entity *entity); /* Must be previously allocated with new. Ownership taken by Engine */
	void destroy(Entity *entity); /* Removes entity from engine and deletes the underlying object */
	void broadcast(EngineEvent *event);
	void quit(void);
	Entity *getMapBoundariesEntity(void);
	/* Actually, it only set a flag. Entities are really destroyed at the end of the physical frame */

	void play(void);
	TextureCache *getTextureCache(void) const;
	sf::RenderWindow &getWindow(void) {return window;}
	const sf::RenderWindow &getWindow(void) const {return window;}
	Vector2d map_size(void);
	void seekCollisions(Entity *entity);

	private:
	Player *getPlayerByJoystickId(int joyid);
	Entity *getEntityByUID(sf::Uint32 uid);
	Player *getPlayerByUID(sf::Uint32 uid);
	Missile *getMissileByUID(sf::Uint32 uid);
	void controller_activity(sf::Event &e);
	bool step(void);
	void draw(void);
	void compute_physics(void);
	void destroy_flagged(void);

	mutable TextureCache texture_cache;

	static const unsigned minFPS;
	static const unsigned maxFPS;
	ControllerDefinitions cdef;
	sf::Font score_font;
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
