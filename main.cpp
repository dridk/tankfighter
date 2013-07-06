#ifdef SFML2
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Audio.hpp>
#else
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <math.h>
#include "json.h"
#include "geometry.h"

#ifdef SFML2
#define LoadFromFile loadFromFile
#else
#define LoadFromFile LoadFromFile
#define Texture Image
#endif

using namespace sf;
using namespace std;

#undef DEBUG_OBJECT_OUTLINES

RenderWindow window;
enum ControllerType {CN_NONE, CN_KEYBMOUSE, CN_JOYSTICK};
enum JoystickAxis {HorizontalMove, VerticalMove, HorizontalDirection, VerticalDirection};


struct Block {
	unsigned short x,y,width,height;
	unsigned short textureid;
};
struct Map {
	std::vector<Block> blocks;
};


void *load_file(const char *input_file_name, unsigned long *file_size) {
	char *file_contents;
	long input_file_size;
	FILE *input_file = fopen(input_file_name, "rb");
	if (!input_file) return NULL;
	fseek(input_file, 0, SEEK_END);
	input_file_size = ftell(input_file);
	*file_size = input_file_size;
	rewind(input_file);
	file_contents = (char*)malloc(input_file_size * (sizeof(char)));
	if (!file_contents) return NULL;
	fread(file_contents, sizeof(char), input_file_size, input_file);
	fclose(input_file);
	return file_contents;
}

json_value *access_json_hash(json_value *p, const json_char *key) {
	if (p->type != json_object) return NULL;
	for (unsigned i=0; i < p->u.object.length; i++) {
		if (strcmp(p->u.object.values[i].name, key)==0) {
			return p->u.object.values[i].value;
		}
	}
	return NULL;
}

#define reterror(err) {fprintf(stderr, "%s\n", err);return;}
bool assertinteger(const char *varname, const json_value *val) {
	if (val->type != json_integer) {
		fprintf(stderr, "Expected integer for parameter %s\n", varname);
		return false;
	}
	return true;
}
bool try_assign_integer_variable(unsigned short *out, const char *varname, const char *key, const json_value *val) {
	if (strcmp(key, varname)==0) {
		if (assertinteger(varname, val)) {*out = val->u.integer;return true;}
		else return false;
	}
	return true;
}
void load_map(Map &map, const char *json_path) {
	unsigned long file_size;
	char *json = (char*)load_file(json_path, &file_size);
	json_value *p = json_parse(json, file_size);
	if (!p) reterror("Failed to parse json");
	
	if (p->type != json_object) reterror("JSON must be an associative array!");
	
	const json_value *map_type = access_json_hash(p, "type");
	if (!map_type) {
		reterror("JSON is not a JSON map (no type field)!");
	}
	if (!(map_type->type == json_string && strcmp(map_type->u.string.ptr, "ktank-map")==0)) {
		reterror("JSON is not a JSON map (type field is not ktank-map)!");
	}
	const json_value *blocks = access_json_hash(p, "blocks");
	if (!blocks) reterror("Map lacks a blocks array!");
	if (!blocks->type == json_array) reterror("Map blocks field should be an array!");
	for(int i=0; i < blocks->u.array.length; i++) {
		const json_value *entity = blocks->u.array.values[i];
		if (entity->type != json_object) {
			fprintf(stderr, "Map block is not hash! Block ignored!");
			continue;
		}
		Block block={0};
		for(int j=0; j < entity->u.object.length; j++) {
			const char *key = entity->u.object.values[j].name;
			const json_value *value = entity->u.object.values[j].value;
			try_assign_integer_variable(&block.x, "x", key, value);
			try_assign_integer_variable(&block.y, "y", key, value);
			try_assign_integer_variable(&block.width, "w", key, value);
			try_assign_integer_variable(&block.height, "h", key, value);
		}
		fprintf(stderr, "[new block found (%d,%d)-(%d,%d)]\n", block.x, block.y, block.width, block.height);
		map.blocks.push_back(block);
	}
}


struct Controller {
	ControllerType type;
	int joyid;
	Joystick::Axis V_axis; /* 0 means unknown */

	float getAxis(JoystickAxis axis);
	bool is_shooting(void);
	private:
	float getVaxis();
	float getJoyAxis(Joystick::Axis axis);
};
struct Player {
	struct Controller controller;
	double tank_x, tank_y, tank_angle, canon_angle;

	Player();
	double speed();
	double tank_rotation();
	double canon_get_angle();
	double canon_rotation();
	double tank_get_angle();
	bool is_canon_absolute_angle();
	bool is_tank_absolute_angle();
	bool is_shooting() {return controller.is_shooting();}

	int playerUID;
	static int UID;
	Color color;
	Clock shoot_clock;
};
struct Missile {
	double x, y;
	double get_dx();
	double get_dy();
	double get_direction();
	int playerUID;
	//private:
	double angle;
	static const double speed=3;
};
struct WorldState {
	std::vector<Player> players;
	std::vector<Missile> missiles;
	Map map;
} wstate;

struct Resources {
	Texture tex1, tex2, tex3, tex4, tex5;
	Sprite tank, canon, missile, background, wall;
	Sound sound;
} wres;

double Missile::get_dx() {
	return cos(angle/180*M_PI)*speed;
}
double Missile::get_dy() {
	return sin(angle/180*M_PI)*speed;
}
double Missile::get_direction() {
	return angle;
}
bool Controller::is_shooting() {
	if (type == CN_KEYBMOUSE) {
		return Mouse::isButtonPressed(Mouse::Left)||Keyboard::isKeyPressed(Keyboard::RControl)||Keyboard::isKeyPressed(Keyboard::LControl);
	} else if (type == CN_JOYSTICK) {
		for(int i=0; i < Joystick::getButtonCount(joyid); i++) {
			if (Joystick::isButtonPressed(joyid, i)) return true;
		}
		if (V_axis != Joystick::Z) { /* manette Xbox */
			if (getJoyAxis(Joystick::Z) > 1e-3 || getJoyAxis(Joystick::R) > 1e-3) return true;
		}
		return false;
	}
	return false;
}

void new_joystick_user(int joyid);
void delete_joystick_user(int joyid);
int player_of_joyid(int joyid);

void player_controller_event(int playerid, const Event &e);

void treat_event(const Event &e) {
  
  
        if (e.type == Event::KeyPressed ) 
	{
	 if (e.key.code == Keyboard::Escape)
	   window.close();
	   
	}
  
	if (e.type == Event::JoystickMoved) {
		printf("joystick %d axis is %d and position is %lf\n"
                      , e.joystickMove.joystickId
	              , e.joystickMove.axis
		      , e.joystickMove.position);
		int joyid = e.joystickMove.joystickId;
		int playerid=player_of_joyid(joyid);
		if (playerid == -1) new_joystick_user(joyid);
		else player_controller_event(playerid, e);
	} else if (e.type == Event::JoystickConnected) {
		new_joystick_user(e.joystickConnect.joystickId);
	} else if (e.type == Event::JoystickDisconnected) {
		delete_joystick_user(e.joystickConnect.joystickId);

	} else if (e.type == Event::JoystickButtonPressed) {
		int joyid = e.joystickButton.joystickId;
		printf("joystick %d button %d pressed\n", joyid, e.joystickButton.button);
		int playerid=player_of_joyid(joyid);
		if (playerid == -1) new_joystick_user(joyid);
		else player_controller_event(playerid, e);
	}
}

void player_controller_event(int playerid, const Event &e) {
	
}


int playerid_from_playerUID(int playerUID) {
	for(int i=0; i < wstate.players.size(); i++) {
		if (wstate.players[i].playerUID == playerUID) return i;
	}
	return -1;
}
Player *player_from_playerUID(int playerUID) {
	int i=playerid_from_playerUID(playerUID);
	if (i < 0) return NULL;
	return &wstate.players[i];
}
int Player::UID;
Player::Player() {
	Controller vide;
	vide.type == CN_NONE;
	vide.joyid = 0;
	vide.V_axis = (Joystick::Axis)0;
	tank_x=tank_y=tank_angle=canon_angle=0;
	controller=vide;
	color=Color(255,255,255);

	playerUID=++UID;
}
bool Player::is_canon_absolute_angle() {
	/*return controller.type == CN_KEYBMOUSE;*/
	return true;
}
bool Player::is_tank_absolute_angle() {
	return controller.type == CN_JOYSTICK;
}
float Controller::getJoyAxis(Joystick::Axis axis) {
	if (Joystick::hasAxis(joyid, axis))
		{
			double d = Joystick::getAxisPosition(joyid, axis);
			if (fabs(d) <= 100.1) return d/100;
			else return 0;
		}
	else return 0;
}
float Controller::getVaxis() {
	if (V_axis == Joystick::Z) return getJoyAxis(Joystick::Z)+getJoyAxis(Joystick::V);
	if (V_axis != 0) return getJoyAxis(V_axis);
	return 0;
}

float Controller::getAxis(JoystickAxis axis) {
	switch(axis) {
		case HorizontalMove:
			return getJoyAxis(Joystick::X)+getJoyAxis(Joystick::PovX);
		case VerticalMove:
			return getJoyAxis(Joystick::Y)+getJoyAxis(Joystick::PovY);
		case HorizontalDirection:
			return getJoyAxis(Joystick::U);
		case VerticalDirection:
			{
			/*fprintf(stderr, "[Z is %lf, V is %lf]\n", getJoyAxis(Joystick::Z), getJoyAxis(Joystick::V));*/
			if (V_axis != 0) return getVaxis();
			/* If it's Xbox controller, Z axis is insane while V axis is sane */
			double z = getJoyAxis(Joystick::Z);
			double v = getJoyAxis(Joystick::V);
			if (fabs(fabs(z)-1)<=1e-3) V_axis=Joystick::V; /* Xbox controller */
			else V_axis=Joystick::Z;
			return getVaxis();
			}
		default:
		return 0;
	}
}
static double speed_from_dxdy(double dx, double dy) {return sqrt(dx*dx+dy*dy);}
static double angle_from_dxdy(double dx, double dy) {
	double angle=0;
	angle = atan(dy/dx);
	if (dx < 0) angle += M_PI;
	return angle;
}
double Player::speed() {
	if (controller.type == CN_KEYBMOUSE) {
		if (Keyboard::isKeyPressed(Keyboard::Up) && !Keyboard::isKeyPressed(Keyboard::Down)) {
			return 1;
		} else if (Keyboard::isKeyPressed(Keyboard::Down) && !Keyboard::isKeyPressed(Keyboard::Up)) {
			return -1;
		} else {
			return 0;
		}
	} else if (controller.type == CN_JOYSTICK) {
		double dx=controller.getAxis(HorizontalMove), dy=controller.getAxis(VerticalMove);
		if (fabs(dx) < 0.2 && fabs(dy) < 0.2) return 0;
		return speed_from_dxdy(dx, dy);
	}
	return 0;
}
double Player::canon_get_angle() {
	if (controller.type == CN_KEYBMOUSE || controller.type == CN_JOYSTICK) {
		double dx, dy;
		if (controller.type == CN_KEYBMOUSE) {
			Vector2i pos = Mouse::getPosition(window);
			dy = pos.y - tank_y;
			dx = pos.x - tank_x;
		} else {
			dy = controller.getAxis(VerticalDirection);
			dx = controller.getAxis(HorizontalDirection);
		}
		double angle=angle_from_dxdy(dx, dy)/M_PI*180;
		if (fabs(dx) < 0.5 && fabs(dy) < 0.5) angle = canon_angle;
		canon_angle = angle;
		return angle;
	}
	return 0;
}
double Player::canon_rotation() {
#if 0
	if (controller.type == CN_JOYSTICK) {
		return controller.getAxis(Joystick::U);
	}
#endif
	return 0;
}
double Player::tank_rotation() {
	if (controller.type == CN_KEYBMOUSE) {
		if (Keyboard::isKeyPressed(Keyboard::Left) && !Keyboard::isKeyPressed(Keyboard::Right)) {
			return -1;
		} else if (Keyboard::isKeyPressed(Keyboard::Right) && !Keyboard::isKeyPressed(Keyboard::Left)) {
			return 1;
		}
		return 0;
	} else if (controller.type == CN_JOYSTICK) {
		return controller.getAxis(HorizontalMove);
	}
	return 0;
}
double Player::tank_get_angle() {
	if (controller.type == CN_JOYSTICK) {
		double dx=controller.getAxis(HorizontalMove), dy=controller.getAxis(VerticalMove);
		double angle = angle_from_dxdy(dx, dy)/M_PI*180;
		if (fabs(dx) < 0.2 && fabs(dy) < 0.2) angle = tank_angle;
		tank_angle = angle;
		return angle;
	}
	return 0;
}

double map_width(void) {
	double sz = window.getSize().x;
	return sz == 0 ? 800 : sz;
}
double map_height(void) {
	double sz = window.getSize().y;
	return sz == 0 ? 600 : sz;
}

void load_sprite(Sprite &sprite, Texture &tex, const char *path)
{
	if (!tex.LoadFromFile(path)) {
		fprintf(stderr, "Failed to load %s\n", path);
		exit(1);
	}
	tex.setSmooth(true);
#ifdef SFML2
	sprite.setTexture(tex);
#else
	sprite.SetImage(tex);
#endif
	FloatRect r=sprite.getLocalBounds();
	sprite.setOrigin(Vector2f(r.width/2, r.height/2));
}
void load_texture(Sprite &sprite, Texture &tex, const char *path) {
	load_sprite(sprite, tex, path);
	sprite.setOrigin(0,0);
	tex.setRepeated(true);
}
void initialize_res(void) {
  
   sf::SoundBuffer buffer;
    if (buffer.loadFromFile("audio.wav"))
      wres.sound.setBuffer(buffer);
  
  	load_sprite(wres.tank, wres.tex1, "sprites/car.png");
	load_sprite(wres.canon, wres.tex2, "sprites/canon.png");
	load_sprite(wres.missile, wres.tex3, "sprites/bullet.png");

	load_texture(wres.background, wres.tex4, "sprites/dirt.jpg");
	FloatRect r=wres.background.getLocalBounds();
	wres.background.setTextureRect(IntRect(0,0,map_width(), map_height()));

	load_texture(wres.wall, wres.tex5, "sprites/bloc2.png");
#if 0
	if (!background.loadFromFile("sprites/dirt.jpg")) {
		fprintf(stderr, "Failed to load %s\n", path);
		exit(1);
	}
#endif
}
double get_random(void) {
	return double(rand())/RAND_MAX;
}
int initial_user_count(void) {
	int count = 0;
	return 1;
	for(int i=0; i < Joystick::Count; i++)
		if (Joystick::isConnected(i)) {
			count++;
			fprintf(stderr, "joystick %d connected\n", i);
		}
		else fprintf(stderr, "joystick %d NOT connected\n", i);
}

void initialize_world(void) {
	load_map(wstate.map, "map2.json");
	wstate.players.reserve(8);
	wstate.players.resize(initial_user_count());
	for(int i=0; i < wstate.players.size(); i++) {
		Player pl;
		if (i==0) pl.controller.type=CN_KEYBMOUSE;
		pl.tank_x = get_random()*map_width();
		pl.tank_y = get_random()*map_height();
		pl.tank_angle = get_random()*360;
		pl.canon_angle = get_random()*360;
		pl.color = Color(get_random()*128+127, get_random()*128+127, get_random()*128+127);
		wstate.players[i] = pl;
	}
}
int player_of_joyid(int joyid) {
	std::vector<Player> &players=wstate.players;
	for(int i=0; i < players.size(); i++) {
		if (players[i].controller.type == CN_JOYSTICK && players[i].controller.joyid == joyid) {
			return i;
		}
	}
	return -1;
}
void new_joystick_user(int joyid) {
	if (player_of_joyid(joyid) >= 0) return;
	Player pl;
	pl.controller.type=CN_JOYSTICK;
	pl.controller.joyid=joyid;
	pl.tank_x = get_random()*800;
	pl.tank_y = get_random()*600;
	pl.tank_angle = get_random()*360;
	pl.canon_angle = get_random()*360;
	pl.color = Color(get_random()*255, get_random()*255, get_random()*255);
	wstate.players.push_back(pl);
}
void delete_joystick_user(int joyid) {
	std::vector<Player> &players=wstate.players;
	int i=player_of_joyid(joyid);
	if (i < 0) return;
	players.erase(players.begin()+i);
}
void draw_player(Player &pl) {
		Sprite &tank=wres.tank;
		tank.setPosition(Vector2f(pl.tank_x, pl.tank_y));
		tank.setRotation(pl.tank_angle+90);
		tank.setColor(pl.color);
		window.draw(tank);
		Sprite &canon=wres.canon;

		canon.setPosition(Vector2f(pl.tank_x, pl.tank_y));
		canon.setRotation(pl.canon_angle+90);
		canon.setColor(pl.color);
		window.draw(canon);

#ifdef DEBUG_OBJECT_OUTLINES
		CircleShape c;
		c.setRadius(64);
		c.setOrigin(64, 64);
		c.setPosition(Vector2f(pl.tank_x, pl.tank_y));
		c.setFillColor(Color(255,0,0));
		c.setOutlineColor(Color(0,0,255));
		window.draw(c);
#endif
}
void draw_missile(Missile &m) {
	Sprite &missile=wres.missile;
	missile.setPosition(Vector2f(m.x, m.y));
	missile.setRotation(m.get_direction()+90);
	Player *pl=player_from_playerUID(m.playerUID);
	if (pl) {
		missile.setColor(pl->color);
		window.draw(missile);
	}
}
void draw_block(Block &b) {
	wres.wall.setPosition(Vector2f(b.x, b.y));
	wres.wall.setTextureRect(IntRect(0,0,b.width,b.height));
	window.draw(wres.wall);
#ifdef DEBUG_OBJECT_OUTLINES
	RectangleShape r;
	r.setFillColor(Color(0,255,0));
	r.setOutlineColor(Color(0,0,255));
	r.setPosition(Vector2f(b.x,b.y));
	r.setSize(Vector2f(b.width, b.height));
	window.draw(r);
#endif
}
void draw_world(sf::RenderWindow &w) {
	for(int i=0; i < wstate.map.blocks.size(); i++) {
		draw_block(wstate.map.blocks[i]);
	}
	for(int i=0; i < wstate.missiles.size(); i++) {
		draw_missile(wstate.missiles[i]);
	}
	for(int i=0; i < wstate.players.size(); i++) {
		draw_player(wstate.players[i]);
	}
}

void display_world(sf::RenderWindow &w) {
	w.clear();
	w.draw(wres.background);
	draw_world(w);
	w.display();
}
void adjust_angle(double &x) {
	if (x > 360) x-=360;
	if (x < 0) x+=360;
}
void move_between_boundaries(double &pos, double rel, double minv, double maxv) {
	if (pos+rel < minv) pos = minv;
	else if (pos+rel > maxv) pos = maxv;
	else pos += rel;
}
void move_player(Player &pl, Int64 tm) {
	double linear_speed = 3e-4 * tm; /* pixel/sec */
	double angle_speed  = 3e-4 * tm; /* degrees/sec */
	if (pl.is_tank_absolute_angle()) {
		pl.tank_angle = pl.tank_get_angle();
	} else {
		pl.tank_angle  += pl.tank_rotation() * angle_speed;
	}

	adjust_angle(pl.tank_angle);
	if (pl.is_canon_absolute_angle()) {
		pl.canon_angle = pl.canon_get_angle();
	} else {
		pl.canon_angle += pl.canon_rotation() * angle_speed;
		adjust_angle(pl.canon_angle);
	}
	Segment vect;
	vect.pt1.x = pl.tank_x;
	vect.pt1.y = pl.tank_y;
	vect.pt2 = vect.pt1;
	move_between_boundaries(vect.pt2.x, cos(pl.tank_angle/180*M_PI) * linear_speed * pl.speed(), 0, map_width());
	move_between_boundaries(vect.pt2.y, sin(pl.tank_angle/180*M_PI) * linear_speed * pl.speed(), 0, map_height());

	std::vector<Block> &blocks = wstate.map.blocks;
	MoveContext ctx;
	ctx.vect = vect;
	ctx.interaction = IT_SLIDE;
	for(unsigned i=0; i < blocks.size(); i++) {
		DoubleRect r;
		Block &block = blocks[i];
		r.left = block.x;
		r.top = block.y;
		r.width = block.width;
		r.height = block.height;
		if (moveCircleToRectangle(64, ctx, r)) {
			/*fprintf(stderr, "[collision on block %d]\n", i);*/;
		}
	}
	ctx.interaction = IT_CANCEL;
	for(unsigned i=0; i < blocks.size(); i++) {
		DoubleRect r;
		Block &block = blocks[i];
		r.left = block.x;
		r.top = block.y;
		r.width = block.width;
		r.height = block.height;
		if (moveCircleToRectangle(64, ctx, r)) {
			/*fprintf(stderr, "[collision on block %d]\n", i);*/;
		}
	}
	vect = ctx.vect;
	pl.tank_x = vect.pt2.x;
	pl.tank_y = vect.pt2.y;
	
	if (!pl.speed()) 
	   wres.sound.play();
}
void create_missile(Player &pl) {
	Missile m= {0};
	m.playerUID = pl.playerUID;
	m.x = pl.tank_x;
	m.y = pl.tank_y;
	m.angle = pl.canon_angle;
	wstate.missiles.push_back(m);
}
void treat_user_actions(Player &pl, Int64 tm) {
	if (pl.is_shooting() && pl.shoot_clock.getElapsedTime().asMilliseconds() >= 200) {
		create_missile(pl);
		pl.shoot_clock.restart();
	}
}
bool is_between(double v, double minv, double maxv) {
	return v >= minv && v < maxv;
}
bool move_missile(Missile &m, Int64 tm) {
	double linear_speed = 3e-4 * tm;
	Player *pl = player_from_playerUID(m.playerUID);
	if (pl == NULL) return false;
	m.x += m.get_dx() * linear_speed;
	m.y += m.get_dy() * linear_speed;
	if (!is_between(m.x, 0, map_width()))
		return false;
	return true;
}

void compute_world(sf::RenderWindow &w, Int64 tm) {
	for(int i=0; i < wstate.players.size(); i++) {
		move_player(wstate.players[i], tm);
		treat_user_actions(wstate.players[i], tm);
	}
	for(int i=0; i < wstate.missiles.size();) {
		if (!move_missile(wstate.missiles[i], tm)) {
			wstate.missiles.erase(wstate.missiles.begin()+i); /* this missile disappears */
		} else i++;
	}
}
int main() {
void test_geometry_cpp();
	test_geometry_cpp();
	srand(time(NULL));
	window.create(VideoMode(1920,1080), "Tank window", Style::Default);
	window.setVerticalSyncEnabled(true);
	window.clear(Color::White);
	initialize_world();
	initialize_res();
	
	
	Clock tm;
	while (window.isOpen()) {
		Event e;
		display_world(window);
		while (window.pollEvent(e)) {
			if (e.type == Event::Closed)
				window.close();
			treat_event(e);
		}
		Int64 elapsed = tm.getElapsedTime().asMicroseconds();
		tm.restart();
		compute_world(window, elapsed);
	}
	return 0;
}
