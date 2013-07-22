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
#include "network_controller.h"
#include "messages.h"

class Entity;
class EngineEvent;
class Player;
class Missile;
class Menu;

class Engine
{
	typedef std::deque<Entity*> Entities;
	public:
	typedef Entities::iterator EntitiesIterator;
	EntitiesIterator begin_entities();
	EntitiesIterator end_entities();
	Engine();
	~Engine();
	NetworkClient *getNetwork(void);
	bool canCreateMissile(Player *pl);
	void defineMapBoundaries(unsigned width, unsigned height);
	void clear_entities(void);
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

	Entity *getEntityByUID(sf::Uint32 uid);
	Player *getPlayerByUID(sf::Uint32 uid);
	Missile *getMissileByUID(sf::Uint32 uid);
	private:
	Player *getPlayerByJoystickId(int joyid);
	void controller_activity(sf::Event &e);
	bool step(void);
	void draw(void);
	void compute_physics(void);
	void destroy_flagged(void);
	void display(const std::string &text, const sf::Color *c = NULL);

	mutable TextureCache texture_cache;

	ControllerDefinitions cdef;
	sf::Font score_font;
	Entity *map_boundaries_entity;
	sf::Texture background_texture;
	sf::Sprite background;
	sf::RenderWindow window;
	Entities entities;
	sf::Clock clock;
	NetworkClient network;
	bool first_step;
	bool must_quit;
	std::vector<ServerInfo> cur_server_info;
	Menu *network_menu;
	Messages messages;
};
#endif
