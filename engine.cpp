#include <math.h>
#include "engine.h"
#include "entity.h"
#include "geometry.h"
#include "engine_event.h"
#include "misc.h"
#include "wall.h"
#include "player.h"
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/Window.hpp>
#include <SFML/Graphics/Text.hpp>
#include <stdio.h>
#include "commands.h"
#include "missile.h"
#include "parameters.h"
#include "menu.h"
#include "messages.h"


using namespace sf;

static bool interacts(Engine *engine, MoveContext &ctx, Entity *a, Entity *b);


struct MissileCounter
{
	size_t i;
	Player *pl;
	MissileCounter(Player *pl0):i(0),pl(pl0) {}
	bool operator()(Missile *e) {
		i += (e->getOwner() == pl);
		return true;
	}
};
template <class EType, class Accumulator>
bool ApplyEntities(Engine *engine, Accumulator &acc) {
	for(Engine::EntitiesIterator it=engine->begin_entities(), e=engine->end_entities(); e != it; ++it) {
		EType *en = dynamic_cast<EType*>(*it);
		if (en) {
			if (!acc(en)) return false;
		}
	}
	return true;
}
size_t CountMissiles(Engine *engine, Player *pl) {
	MissileCounter counter(pl);
	ApplyEntities<Missile>(engine, counter);
	return counter.i;
}
bool Engine::canCreateMissile(Player *pl) {
	return CountMissiles(this, pl) < 3;
}
void Engine::play(void) {
	network.discoverServers(false);
	while (step()) {
		Event e;
		while (window.pollEvent(e)) {
			if (e.type == Event::Closed) {
				quit();
			} else if (e.type == Event::KeyPressed) {
				if (e.key.code == Keyboard::Escape) quit();
				if (e.key.code == Keyboard::M) {
					char buffer[256];
					const char *words[]={"hello", "this", "stuff", "foo", "bar", "baz", "cool", "rocks", "good"};
					static int n=0;
					sprintf(buffer, "%s %s %s %d", words[int(get_random(sizeof(words)/sizeof(words[0])))]
						, words[int(get_random(sizeof(words)/sizeof(words[0])))]
						, words[int(get_random(sizeof(words)/sizeof(words[0])))]
						, n++);
					display(buffer);
				}
#if 0
				if (e.key.code == Keyboard::J && network.isLocal()) {
					network.discoverServers(false);
				}
				if (e.key.code == Keyboard::K && network.isLocal()) {
					network.declareAsServer();
					destroy_flagged();
				}
#endif
			} else if (e.type == Event::JoystickConnected) {
				addPlayer(0, e.joystickConnect.joystickId);
			} else if (e.type == Event::JoystickDisconnected) {
				Player *pl = getPlayerByJoystickId(e.joystickConnect.joystickId);
				if (pl) {destroy(pl);destroy_flagged();}
			}
			if (KeymapController::maybeConcerned(e)) {
				controller_activity(e);
			}
#ifdef DEBUG_JOYSTICK
			if (e.type == Event::JoystickMoved) {
				fprintf(stderr, "[moved axis %u %u %f]\n", e.joystickMove.joystickId, e.joystickMove.axis, e.joystickMove.position);
			}
			if (e.type == Event::JoystickButtonPressed) {
				fprintf(stderr, "[pressed joybutton %u %u]\n", e.joystickMove.joystickId, e.joystickButton.button);
			}
#endif
		}
	}
	network.requestDisconnection();
}

Controller *decapsulateController(Controller *ctrl) {
	if (!ctrl) return NULL;
	if (MasterController *mctrl = dynamic_cast<MasterController*>(ctrl)) {
		return mctrl->getOrigController();
	}
	return ctrl;
}
void Engine::controller_activity(Event &e) {
	for(EntitiesIterator it=entities.begin(); it != entities.end(); it++) {
		Player *pl = dynamic_cast<Player*>(*it);
		if (!pl) continue;
		LocalController *c = dynamic_cast<LocalController*>(decapsulateController(pl->getController()));
		if (c) {
			if (c->isConcerned(e)) return;
			continue;
		}
	}
	if (network.isPendingPlayerConcerned(e)) return;
	/* this key event is not owned by a player, have a look at controller templates */
	std::vector<KeymapController*> &cd = cdef.forplayer;
	for(unsigned i = 0; i < cd.size(); i++) {
		int ojoyid = -1;
		if (cd[i] && cd[i]->isConcernedAndAffects(e, ojoyid)) {
			addPlayer(i, ojoyid);
			break;
		}
	}
}
Player *Engine::getPlayerByJoystickId(int joyid) {
	for(EntitiesIterator it=entities.begin(); it != entities.end(); it++) {
		Player *pl = dynamic_cast<Player*>(*it);
		if (!pl) continue;
		Controller *ctrl = decapsulateController(pl->getController());
		KeymapController *c = dynamic_cast<KeymapController*>(ctrl);
		if (c) {
			if (c->getJoystickId() == joyid) return pl;
			continue;
		}
		JoystickController *j = dynamic_cast<JoystickController*>(ctrl);
		if (j) {
			if (j->getJoystickId() == joyid) return pl;
		}
	}
	return NULL;
}
Entity *Engine::getEntityByUID(Uint32 uid) {
	for(EntitiesIterator it=entities.begin(); it != entities.end(); it++) {
		if ((*it)->getUID() == uid) return (*it);
	}
	return NULL;
}
Player *Engine::getPlayerByUID(Uint32 uid) {
	return dynamic_cast<Player*>(getEntityByUID(uid));
}
Missile *Engine::getMissileByUID(Uint32 uid) {
	return dynamic_cast<Missile*>(getEntityByUID(uid));
}
void Engine::addPlayer(unsigned cid, int joyid) {
	Controller *newc;
	if (cid >= cdef.forplayer.size()) return;
	if (cid == 0 && joyid ==  -1) {
		/* search a free joystick */
		for(unsigned i=0; i < Joystick::Count; i++) {
			if (Joystick::isConnected(i) && !getPlayerByJoystickId(i)) {
				joyid = i;
				break;
			}
		}
	}
	if (!cdef.forplayer[cid]) {
		fprintf(stderr, "Error: no controller %d found for new player\n", cid);
		return;
	}
	if (cid > 0) joyid = -1;
	if (cid == 0) newc = new JoystickController(joyid);
	else newc = cdef.forplayer[cid]->clone(joyid);
	
	if (network.isLocal() || network.discoveringServers()) {
		add(new Player(newc, this));
	} else if (network.isServer()) {
		Player *pl = new Player(new MasterController(&network, newc), this);
		network.reportNewPlayer(pl);
		add(pl);
	} else if (network.isClient()) {
		network.requestPlayerCreation(newc);
	}
}
Entity *Engine::getMapBoundariesEntity() {
	return map_boundaries_entity;
}
TextureCache *Engine::getTextureCache(void) const {
	return &texture_cache;
}
Vector2d Engine::map_size(void) {
	Vector2u sz = window.getSize();
	return Vector2d(sz.x, sz.y);
}
Engine::EntitiesIterator Engine::begin_entities() {
	return entities.begin();
}
Engine::EntitiesIterator Engine::end_entities() {
	return entities.end();
}
#if 0
static void add_test_menu(Engine *engine) {
	Menu *m = new Menu(engine);
	m->addItem("Hello world!");
	m->addItem("How are you?");
	m->addItem("This is a pretty long string!");
	m->addItem("This is an even longer string that should reduce char size!");
	for(size_t i=0;i<200;i++) {
		char buffer[256];
		sprintf(buffer, "This is item %u", (unsigned)i);
		m->addItem(buffer);
	}
	engine->add(m);
}
#endif
Menu *networkMenu(Engine *engine, const std::vector<ServerInfo> &si) {
	Menu *m = new Menu(engine);
	m->addItem("Play locally");
	m->addItem("Create server");
	for(size_t i=0; i < si.size(); ++i) {
		RemoteClient rc=si[i].remote;
		char portbuffer[64];
		sprintf(portbuffer, "%d", rc.port);
		std::string title = (std::string("Join ")+si[i].name+" at "+rc.addr.toString()+":"+portbuffer);
		fprintf(stderr, "Adding menu item %s\n", title.c_str());
		m->addItem(title.c_str());
	}
	engine->add(m);
	return m;
}
Engine::Engine():network(this),messages(this) {
	network_menu = NULL;
	map_boundaries_entity = NULL;
	first_step = true;
	must_quit = false;
	window.create(VideoMode(1920,1080), "Tank window", Style::Default);
	window.setVerticalSyncEnabled(false);
	window.setFramerateLimit(0);
	window.clear(Color::White);
	score_font.loadFromFile("/usr/share/fonts/truetype/droid/DroidSans.ttf");

	load_texture(background, background_texture, "sprites/dirt.jpg");
	Vector2d sz = map_size();
	defineMapBoundaries(sz.x, sz.y);
	load_keymap(cdef, "keymap.json");
}
void Engine::defineMapBoundaries(unsigned width, unsigned height) {
	if (map_boundaries_entity) {
		delete map_boundaries_entity;
		map_boundaries_entity = NULL;
	}
	background.setTextureRect(IntRect(0,0,width, height));
	map_boundaries_entity = new Wall(0,0,width, height, NULL, this);
	add(map_boundaries_entity);
}
void Engine::clear_entities(void) {
	for(EntitiesIterator it=entities.begin(); it != entities.end(); ++it) {
		delete (*it);
	}
	map_boundaries_entity = NULL;
	entities.resize(0);
}
Engine::~Engine() {
	clear_entities();
	window.close();
}
void Engine::seekCollisions(Entity *entity) {
	int i=100;
	bool retry = false;
	do {
	for(EntitiesIterator it=entities.begin(); it != entities.end(); ++it) {
		Entity *centity = (*it);
		Segment vect;
		vect.pt1 = entity->position;
		vect.pt2 = entity->position;
		MoveContext ctx(IT_GHOST, vect);
		if (interacts(this, ctx, entity, centity)) {
			CollisionEvent e;
			e.first = entity;
			e.second = centity;
			e.interaction = IT_GHOST;
			broadcast(&e);
			if (e.retry) retry = true;
		}
	}
	} while(retry && --i > 0);
}
void Engine::add(Entity *entity) {
	entity->setEngine(this);
	entities.push_back(entity);
	/* signal spawn-time collisions */
	seekCollisions(entity);
}
void Engine::destroy(Entity *entity) { /* Removes entity from engine and deletes the underlying object */
	entity->setKilled();
}
void Engine::broadcast(EngineEvent *event) {
	/* assume iterators may be invalidated during event processing, because new items may be added */
	/* note that entities may be set to killed, but cannot be removed from the list (they are only flagged) */

	for(size_t i=0; i < entities.size(); ++i) {
		Entity *entity = entities[i];
		if (!entity->isKilled()) entity->event_received(event);
	}
}
void Engine::quit(void) {
	must_quit = true;
}
bool Engine::step(void) {
	if (must_quit) return false;
	if (first_step) {clock.restart();first_step=false;}
	draw();
	compute_physics();
	if (network_menu) {
		if (network_menu->selectionValidated()) {
			int icl = network_menu->getSelected();
			fprintf(stderr, "network selection %d\n", icl);
			destroy(network_menu);
			network_menu = NULL;
			if (icl == 1) {
				network.declareAsServer();
			} else if (icl >= 2) {
				network.requestConnection(cur_server_info[icl-2].remote);
			}
		}
	}
	if (!network.isLocal()) {
		network.transmitToServer();
		network.receiveFromServer();
		if ((!network_menu) && network.discoveringServers() && network.shouldEndDiscovery()) {
			network.endServerDiscovery();
			size_t i=0;
			ServerInfo si;
			cur_server_info.clear();
			for(NetworkClient::ServerInfoIterator it=network.begin_servers(), en=network.end_servers(); it != en; ++it) {
				si = *it;
				fprintf(stderr, "Discovered server %s at %s:%d\n"
					,si.name.c_str()
					,si.remote.addr.toString().c_str(), si.remote.port);
				cur_server_info.push_back(*it);
				i++;
			}
			network_menu = networkMenu(this, cur_server_info);
#if 0
			if (i==1) {
				network.requestConnection(si.remote);
			}
#endif
		}
	}
	destroy_flagged();
	return !must_quit;
}
static void draw_score(RenderTarget &target, Font &ft, int score, Color color, Vector2d pos) {
	Text text;
	char sscore[256];
	sprintf(sscore, "%d", score);
	text.setCharacterSize(128);
	text.setString(sscore);
	text.setColor(Color(0,0,0));
	text.setPosition(Vector2f(pos.x+2, pos.y+2));
	text.setFont(ft);
	target.draw(text);

	text.setColor(color);
	text.setPosition(Vector2f(pos.x, pos.y));

	target.draw(text);
}
void Engine::draw(void) {
	Vector2d scorepos;
	scorepos.x = 16;
	scorepos.y = 16;
	window.clear();
	window.draw(background);
	/* draw walls before other entities */
	for(EntitiesIterator it=entities.begin(); it != entities.end(); ++it) {
		Wall *wall = dynamic_cast<Wall*>(*it);
		if (wall) wall->draw(window);
	}
	/* then, draw players (before missiles) */
	for(EntitiesIterator it=entities.begin(); it != entities.end(); ++it) {
		if (Player *pl = dynamic_cast<Player*>((*it))) {
			pl->draw(window);
			draw_score(window, score_font, pl->getScore(), pl->getColor(), scorepos);
			scorepos.x += 384+16;
		}
	}
	/* then, draw other entities */
	for(EntitiesIterator it=entities.begin(); it != entities.end(); ++it) {
		Entity *e=*it;
		if (dynamic_cast<Player*>(e) || dynamic_cast<Wall*>(e)) continue;
		e->draw(window);
	}
	messages.movement(0);
	messages.draw(window);
	window.display();
}
static double getEntityRadius(Entity *a) {
	return a->getSize().x/2;
}
static DoubleRect getEntityBoundingRectangle(Entity *a) {
	Vector2d pos = a->position;
	Vector2d size = a->getSize();
	DoubleRect r;
	r.left = pos.x;
	r.top  = pos.y;
	r.width  = size.x;
	r.height = size.y;
	return r;
}
static Circle getEntityCircle(Entity *a) {
	Circle circle;
	circle.filled = true;
	circle.center = a->position;
	circle.radius = getEntityRadius(a);
	return circle;
}
/* Note: Dynamic entities must be circle-shaped */
static bool interacts(Engine *engine, MoveContext &ctx, Entity *a, Entity *b) {
	if (a->shape == SHAPE_CIRCLE && b->shape == SHAPE_RECTANGLE) {
		GeomRectangle gr;
		gr.filled = (b != engine->getMapBoundariesEntity());
		gr.r = getEntityBoundingRectangle(b);
		return moveCircleToRectangle(getEntityRadius(a), ctx, gr);
	} else if (a->shape == SHAPE_CIRCLE && b->shape == SHAPE_CIRCLE) {
		return moveCircleToCircle(getEntityRadius(a), ctx, getEntityCircle(b));
	} else {
		return false;
	}
}
bool moveCircleToRectangle(double radius, MoveContext &ctx, const DoubleRect &r);
bool moveCircleToCircle(double radius, MoveContext &ctx, const Circle &colli);

static bool quasi_equals(double a, double b) {
	return fabs(a-b) <= 1e-3*(fabs(a)+fabs(b));
}
void Engine::compute_physics(void) {
	unsigned minFPS = parameters.minFPS();
	unsigned maxFPS = parameters.maxFPS();
	Int64 tm = clock.getElapsedTime().asMicroseconds();
	if (tm < 1000000L/maxFPS) {
		unsigned long rtm = 1000000L/maxFPS - tm;
		if (rtm < 100) return;
		sf::sleep(microseconds(rtm - 100));
		return;
	}
	if (tm == 0) return;
	if (tm > 1000000L/minFPS) tm = 1000000L/minFPS;
	clock.restart();
	for(EntitiesIterator it=entities.begin(); it != entities.end(); ++it) {
		Entity *entity = (*it);
		if (entity->isKilled()) continue;
		Vector2d movement = entity->movement(tm);
		Vector2d old_speed = movement;
		old_speed.x /= tm;
		old_speed.y /= tm;
		Segment vect;
		vect.pt1 = entity->position;
		vect.pt2 = vect.pt1 + movement;
		if ((fabs(movement.x)+fabs(movement.y)) < 1e-4) continue;
		MoveContext ctx(IT_GHOST, vect);
		for(int pass=0; pass < 2; ++pass)
		for (EntitiesIterator itc=entities.begin(); itc != entities.end(); ++itc) {
			Entity *centity = *itc;
			if (centity->isKilled()) continue;
			if (entity->isKilled()) break;
			if (centity == entity) continue;
			ctx.interaction = IT_GHOST;
			MoveContext ctxtemp = ctx;
			if (interacts(this, ctxtemp, entity, centity)) {
				if (entity->isKilled() || centity->isKilled()) continue;
				CollisionEvent e;
				e.type   = COLLIDE_EVENT;
				e.first  = entity;
				e.second = centity;
				e.interaction = IT_GHOST; /* default interaction type */
				broadcast(&e); /* should set e.interaction */
				ctx.interaction = e.interaction;
				if (pass == 1 && ctx.interaction != IT_GHOST) {
					/* On second interaction in the same frame, cancel movement */
					ctx.vect.pt2 = ctx.vect.pt1 = entity->position;
					break;
				}
				interacts(this, ctx, entity, centity);
			}
		}
		if (entity->isKilled()) continue;
		vect = ctx.vect;
		CompletedMovementEvent e;
		e.type = COMPLETED_MOVEMENT_EVENT;
		e.entity = entity;
		e.position = vect.pt2;
		Vector2d new_speed = ctx.nmove;
		new_speed.x /= tm;
		new_speed.y /= tm;
		if (quasi_equals(old_speed.x, new_speed.x) && quasi_equals(old_speed.y, new_speed.y)) {
			e.new_speed = old_speed;
			e.has_new_speed = false;
		} else {
			e.new_speed = new_speed;
			e.has_new_speed = true;
		}
		broadcast(&e);
	}
}
void Engine::destroy_flagged(void) {
	bool some_got_deleted;
	do {
		some_got_deleted = false;
		for(size_t i=0; i < entities.size(); ++i) {
			Entity *entity = entities[i];
			if (entity->isKilled()) {
				some_got_deleted = true;
				EntityDestroyedEvent e;
				e.type = ENTITY_DESTROYED_EVENT;
				e.entity = entity;
				broadcast(&e);
				entities.erase(entities.begin()+i);
				delete entity;
				--i;
			}
		}
	} while(some_got_deleted);
#if 0
	if (some_got_deleted) fprintf(stderr, "[now, I'm going to physically destroy things]\n");
	for(size_t i=0; i < entities.size(); ++i) {
		Entity *entity = entities[i];
		if (entity->isKilled()) {
			entities.erase(entities.begin()+i);
			delete entity;
			--i;
		}
	}
	if (some_got_deleted) fprintf(stderr, "[/physically destroyed things]\n");
#endif
}

NetworkClient *Engine::getNetwork(void) {return &network;}

void Engine::display(const std::string &text, const sf::Color *c) {
	messages.display(text, c);
}
