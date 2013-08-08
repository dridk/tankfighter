#ifndef __ENGINE_H__
#define __ENGINE_H__
#include <deque>
#include "coretypes.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/View.hpp>
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
	Vector2d getMousePosition(void) const;
	Vector2d map2window(const Vector2d &p) const;
	Vector2d window2map(const Vector2d &p) const;
	double map_aspect() const;
	void defineMapSize(double width, double height);
	void window_resized(const sf::Vector2i &sz);
	bool canCreateMissile(Player *pl);
	void clear_entities(void);
	void addPlayer(unsigned controllerType, int joyid=-1);
	void add(Entity *entity); /* Must be previously allocated with new. Ownership taken by Engine */
	void destroy(Entity *entity); /* Removes entity from engine and deletes the underlying object */
	void broadcast(EngineEvent *event);
	void quit(void);
	Entity *getMapBoundariesEntity(void);
	/* Actually, it only set a flag. Entities are really destroyed at the end of the physical frame */

	void play(void);
	void loadMap(const char *path);
	TextureCache *getTextureCache(void) const;
	sf::RenderWindow &getWindow(void) {return window;}
	const sf::RenderWindow &getWindow(void) const {return window;}
	Vector2d map_size(void) const;
	void seekCollisions(Entity *entity);

	Entity *getEntityByUID(sf::Uint32 uid);
	Player *getPlayerByUID(sf::Uint32 uid);
	Missile *getMissileByUID(sf::Uint32 uid);
	void display(const std::string &text, const sf::Color *c = NULL);
	private:
	void map_boundaries_changed(void);
	void addJoinItem(const ServerInfo &si);
	Player *getPlayerByJoystickId(int joyid);
	void controller_activity(sf::Event &e);
	bool step(void);
	void draw(void);
	void compute_physics(void);
	void destroy_flagged(void);
	void PopupMenu(void);
	void CheckMenu(void);
	void CloseMenu(void);
	void toggleFullscreen(void);

	mutable TextureCache texture_cache;

	Vector2d msize;
	ControllerDefinitions cdef;
	sf::Font score_font;
	Entity *map_boundaries_entity;
	sf::Texture background_texture;
	sf::Sprite background;
	sf::View view;
	sf::RenderWindow window;
	Entities entities;
	sf::Clock clock;
	NetworkClient network;
	bool first_step;
	bool must_quit;
	std::vector<ServerInfo> cur_server_info;
	Menu *network_menu;
	Messages messages;
	bool is_fullscreen;
};
#endif
