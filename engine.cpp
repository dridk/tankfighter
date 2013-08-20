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
#include "input.h"

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
Vector2d Engine::getMousePosition() const {
	Vector2i p = Mouse::getPosition(window);
	return window2map(Vector2d(p.x, p.y));
}
bool Engine::canCreateMissile(Player *pl) {
	return CountMissiles(this, pl) < 3;
}
double Engine::map_aspect() const {
	Vector2d msz = map_size();
	return msz.x/msz.y;
}
void Engine::window_resized(const Vector2i &wsz) {
	double ratio = map_aspect();
	FloatRect r;
	double w1 = wsz.x;
	double w2 = wsz.y * ratio;
	if (w2 < w1) r.width = w2; else r.width = w1;
	r.height = r.width / ratio;
	r.left = (wsz.x - r.width)/2;
	r.top = (wsz.y - r.height)/2;
	r = FloatRect(r.left/wsz.x, r.top/wsz.y, r.width/wsz.x, r.height/wsz.y);
	view.setViewport(r);
	window.setView(view);
}
void Engine::initialActions() {
	if (parameters.startServer()) {
		network.declareAsServer();
	} else if (parameters.joinAddress() != "") {
		network.requestConnection(string2remote(parameters.joinAddress()));
	}
}
void Engine::play(void) {
	initialActions();
	while (step()) {
		Event e;
		if (!parameters.noGUI()) {
		while (window.pollEvent(e)) {
			treatLocalKeyEvent(e);
			if (e.type == Event::Closed) {
				quit();
			} else if (e.type == Event::KeyPressed) {
				if (e.key.code == Keyboard::Escape) {
					if (network_menu) network_menu->escape();
					else PopupMenu();
				}
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
			} else if (e.type == Event::Resized) {
				window_resized(Vector2i(e.size.width, e.size.height));
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
Vector2d Engine::map_size(void) const {
	return msize;
}
Vector2d Engine::map2window(const Vector2d &pos) const {
	if (parameters.noGUI()) return pos;
	Vector2u wsz = window.getSize();
	Vector2d msz = map_size();
	FloatRect vr = view.getViewport();
	return Vector2d(wsz.x*(pos.x/msz.x * vr.width + vr.left)
	              , wsz.y*(pos.y/msz.y * vr.height + vr.top));
}
Vector2d Engine::window2map(const Vector2d &pos) const {
	if (parameters.noGUI()) return pos;
	Vector2u wsz = window.getSize();
	Vector2d msz = map_size();
	FloatRect vr = view.getViewport();
	return Vector2d(msz.x*(pos.x/wsz.x - vr.left)/vr.width
	               ,msz.y*(pos.y/wsz.y - vr.top)/vr.height);
}
Engine::EntitiesIterator Engine::begin_entities() {
	return entities.begin();
}
Engine::EntitiesIterator Engine::end_entities() {
	return entities.end();
}
void Engine::addJoinItem(const ServerInfo &si) {
	if (!network_menu) return;
	const RemoteClient &rc = si.remote;
	char portbuffer[64];
	sprintf(portbuffer, "%d", rc.port);
	std::string title = (std::string("Join ")+si.name+" at "+rc.addr.toString()+":"+portbuffer);
	network_menu->addItem(title.c_str(), NULL, network_menu->getItemCount()-1);
}
void Engine::PopupMenu(void) {
	if (parameters.noGUI()) return;
	if (network_menu) return;
	Menu *m = new Menu(this);
	network_menu = m;
	m->addItem("Resume game");
	m->addItem("Create server");
	m->addItem("Toggle fullscreen");
	m->addItem("Quit game");
	for(size_t i=0; i < cur_server_info.size(); ++i) {
		addJoinItem(cur_server_info[i]);
	}
	add(m);
	network.discoverServers(false);
}
void Engine::CloseMenu(void) {
	network.endServerDiscovery();
	destroy(network_menu);
	network_menu = NULL;
}
void Engine::toggleFullscreen(void) {
	if (parameters.noGUI()) return;
	VideoMode mode = parameters.getVideoMode();
	if (is_fullscreen) {
		window.create(mode, "Tank window", Style::Default);
	} else {
		window.create(mode, "Tank window", Style::Fullscreen);
	}
	map_boundaries_changed();
	is_fullscreen = !is_fullscreen;
}
void Engine::CheckMenu(void) {
	if (network_menu) {
		if (network_menu->selectionValidated()) {
			int icl = network_menu->getSelected();
			CloseMenu();
			if (icl == 1) {
				network.declareAsServer();
			} else if (icl == 2) {
				toggleFullscreen();
			} else if (icl >= 3) {
				icl -= 3;
				if (unsigned(icl) >= cur_server_info.size()) {
					icl -= cur_server_info.size();
					if (icl == 0) quit();
				}
				else network.requestConnection(cur_server_info[icl].remote);
			}
		}
	}
}
Engine::Engine():network(this),messages(this) {
	msize.x = 1920;
	msize.y = 1080;
	network_menu = NULL;
	map_boundaries_entity = NULL;
	first_step = true;
	must_quit = false;
	is_fullscreen = parameters.fullscreen();
	VideoMode mode = parameters.getVideoMode();
	if (!parameters.noGUI()) {
		window.create(mode, "Tank window", (is_fullscreen ? Style::Fullscreen : Style::Default));
		window.setVerticalSyncEnabled(false);
		window.setFramerateLimit(parameters.maxFPS());
		window.clear(Color::White);
	}
	score_font.loadFromFile(getDefaultFontPath().c_str());

	map_boundaries_changed();
	load_keymap(cdef, parameters.keymap().c_str());
}
void Engine::defineMapSize(double width, double height) {
	msize.x = width; msize.y = height;
	map_boundaries_changed();
}
void Engine::map_boundaries_changed(void) {
	Vector2d msz = map_size();
	double width = msz.x, height = msz.y;

	if (map_boundaries_entity) {
		destroy(map_boundaries_entity);
		map_boundaries_entity = NULL;
	}
	map_boundaries_entity = new Wall(0,0,width, height, 0, parameters.defaultBackgroundTexture().c_str(), this);
	add(map_boundaries_entity);

	if (!parameters.noGUI()) {
		Vector2u wsz = window.getSize();
		view.reset(FloatRect(0,0,wsz.x,wsz.y));
		view.setCenter(width/2, height/2);
		view.setSize(width, height);
		window.setView(view);
		window_resized(Vector2i(wsz.x, wsz.y));
	}
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
		if (entity != centity && interacts(this, ctx, entity, centity)) {
			CollisionEvent e;
			e.first = entity;
			e.second = centity;
			e.interaction = IT_GHOST;
			broadcast(&e);
			if (e.retry) retry = true;
			break;
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
template <class T>
bool insertInArray(const T &obj, std::vector<T> &v) {
	for(unsigned i=0; i < v.size(); i++) {
		if (v[i] == obj) return false;
	}
	v.push_back(obj);
	return true;
}
bool Engine::step(void) {
	if (must_quit) return false;
	if (first_step) {clock.restart();first_step=false;}
	if (parameters.noGUI()) {
		Int64 total   = parameters.C2S_Packet_interval_US();
		Int64 elapsed = clock.getElapsedTime().asMicroseconds();
		if (elapsed < total) {
			sf::sleep(sf::microseconds(total - elapsed));
		}
	} else {
		draw();
	}
	compute_physics();
	CheckMenu();
	if (!network.isLocal()) {
		network.transmitToServer();
		network.receiveFromServer();
		if (network.discoveringServers()) {
			for(NetworkClient::ServerInfoIterator it=network.begin_servers(), en=network.end_servers(); it != en; ++it) {
				if (insertInArray(*it, cur_server_info)) {
					if (network_menu) addJoinItem(*it);
					ServerInfo si;
					si = *it;
				}
			}
			network.discoverMoreServers();
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
#ifdef DEBUG_OUTLINE
static GeomPolygon wall2geompoly(Wall *w) {
	GeomPolygon gr;
	gr.filled = true;
	w->getPolygon(gr.polygon);
	return gr;
}
#endif
void Engine::draw(void) {
	Vector2d scorepos;
	scorepos.x = 16;
	scorepos.y = 16;
	window.clear();
	/* draw walls before other entities */
	map_boundaries_entity->draw(window);
	for(EntitiesIterator it=entities.begin(); it != entities.end(); ++it) {
		if ((*it) == map_boundaries_entity) continue;
		Wall *wall = dynamic_cast<Wall*>(*it);
		if (wall) wall->draw(window);
#ifdef DEBUG_OUTLINE
		extern void drawGeomPolygon(RenderTarget &target, const GeomPolygon &geom, double augment);
		if (wall) drawGeomPolygon(window, wall2geompoly(wall), 64);
#endif
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
	if (a->shape == SHAPE_EMPTY || b->shape == SHAPE_EMPTY) return false;
	if (a->shape == SHAPE_CIRCLE && b->shape == SHAPE_POLYGON) {
		GeomPolygon gr;
		gr.filled = (b != engine->getMapBoundariesEntity());
		b->getPolygon(gr.polygon);
		return moveCircleToPolygon(getEntityRadius(a), ctx, gr);
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
	Int64 tm = clock.getElapsedTime().asMicroseconds();
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
void Engine::loadMap(const char *path) {
	load_map(this, path);
}
