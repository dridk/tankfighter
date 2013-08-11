#include "load_map.h"
#include "misc.h"
#include "json.h"
#include "wall.h"
#include <stdio.h>
#include <stdlib.h>
#include "controller.h"
#include "commands.h"
#include <string>
#include <sstream>
#include "engine.h"
#include "misc.h"
#include "parameters.h"
#include "parse_json.h"

#if 0
struct Block {
	unsigned short x,y,width,height;
	char *texture_name;
};
#endif
class BlockEnumerator {
	public:
	BlockEnumerator(Engine *engine);
	void enumerate(const Block &block);
	private:
	Engine *engine;
};
static void enum_map(BlockEnumerator *blockenum, Vector2d &map_size, const char *file_path);

#define reterror(err) {fprintf(stderr, "%s\n", err);return;}
static void enum_map(BlockEnumerator *blockenum, Vector2d &map_size, const char *json_path) {
	json_value *p = json_parse_file(json_path);
	if (!p) reterror("Failed to parse json");
	
	if (!json_check_magic(p, parameters.map_magic().c_str())) {
		json_value_free(p);
		return;
	}
	const json_value *width = access_json_hash(p, "width");
	const json_value *height = access_json_hash(p, "height");
	if (!(width && width->type == json_integer)) {
		reterror("Map width must be specified as an integer");
	} else map_size.x = width->u.integer;
	if (!(height && height->type == json_integer)) {
		reterror("Map width must be specified as an integer");
	} else map_size.y = height->u.integer;
	const json_value *blocks = access_json_hash(p, "blocks");
	if (!blocks) reterror("Map lacks a blocks array!");
	if (!blocks->type == json_array) reterror("Map blocks field should be an array!");
	for(size_t i=0; i < blocks->u.array.length; i++) {
		const json_value *entity = blocks->u.array.values[i];
		if (entity->type != json_object) {
			fprintf(stderr, "Map block is not hash! Block ignored!");
			continue;
		}
		Block block={0};
		block.texture_name = NULL;
		for(size_t j=0; j < entity->u.object.length; j++) {
			const char *key = entity->u.object.values[j].name;
			const json_value *value = entity->u.object.values[j].value;
			try_assign_integer_variable(&block.x, "x", key, value);
			try_assign_integer_variable(&block.y, "y", key, value);
			try_assign_integer_variable(&block.width, "w", key, value);
			try_assign_integer_variable(&block.height, "h", key, value);
			if (strcmp(key, "texture")==0 && value->type == json_string) block.texture_name = json_string_to_cstring(value);
		}
		blockenum->enumerate(block);
		if (block.texture_name) {free(block.texture_name);block.texture_name=NULL;}
		fprintf(stderr, "[new block found (%d,%d)-(%d,%d)]\n", block.x, block.y, block.width, block.height);
	}
	json_value_free(p);
}
class KeymapEnumerator
{
	public:
	ControllerDefinitions *controllers;
	void enumerate(const char *control, const char *command);
};
static bool enum_keymap(KeymapEnumerator *kmenum, const char *json_path) {
	json_value *p;
	if (!(p=json_parse_file(json_path))) {
		return false;
	}
	if (!json_check_magic(p, parameters.keymap_magic().c_str())) {
		json_value_free(p);
		return false;
	}
	const json_value *map = access_json_hash(p, "CommandMap");
	if ((!map) || map->type != json_object) {
		json_value_free(p);
		fprintf(stderr, "CommandMap must be present and must be an associative array\n");
		return false;
	}
	for (unsigned i=0; i < map->u.object.length; i++) {
		const char *control=map->u.object.values[i].name;
		const json_value *val = map->u.object.values[i].value;
		char *command = json_string_to_cstring(val);
		if (!command) {
			fprintf(stderr, "Command must be represented as a string\n");
			continue;
		}
		kmenum->enumerate(control, command);
		free(command);
	}
	return true;
}


void KeymapEnumerator::enumerate(const char *control, const char *command) {
	if (!command) return;
	if (!control) {
		fprintf(stderr, "Critical error: null trigger\n");
		return;
	}
	if (strlen(command) == 0) return;
	std::string cmd = command;
	std::string ktype;
	std::istringstream in(control);
	unsigned last = command[strlen(command)-1];
	unsigned idPlayer = 0;
	if (isdigit(last)) {
		idPlayer = last - '0';
		cmd.resize(cmd.size()-1);
	}
	if (idPlayer == 0 && (in >> ktype) && LowerCaseString(ktype) != "joy" && LowerCaseString(ktype) != "joystick") { /* keyboard or mouse */
		idPlayer = 1;
	}
	/* reallocate vector */
	std::vector<KeymapController*> &fp=controllers->forplayer;
	if (fp.size() <= idPlayer) fp.resize(idPlayer+1);
	if (!fp[idPlayer]) fp[idPlayer] = new KeymapController;

	fprintf(stderr, "map %s to %s for player %d\n", control, cmd.c_str(), idPlayer);
	fp[idPlayer]->mapControl(control, cmd.c_str());
}


void load_keymap(ControllerDefinitions &controllers, const char *file_path) {
	KeymapEnumerator ke;
	ke.controllers = &controllers;
	enum_keymap(&ke, file_path);
}

BlockEnumerator::BlockEnumerator(Engine *engine0)
	:engine(engine0)
{
}
void load_map(Engine *engine, const char *file_path) {
	Vector2d map_size;
	BlockEnumerator blockenum(engine);
	enum_map(&blockenum, map_size, file_path);
	engine->defineMapSize(map_size.x, map_size.y);
}

void BlockEnumerator::enumerate(const Block &block) {
	engine->add(new Wall(block.x, block.y, block.width, block.height, block.texture_name, engine));
}

ControllerDefinitions::ControllerDefinitions() {}
ControllerDefinitions::~ControllerDefinitions() {
	for(unsigned i=0; i < forplayer.size(); i++) {
		if (forplayer[i]) delete forplayer[i];
	}
}
