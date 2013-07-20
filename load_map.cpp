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
static void enum_map(BlockEnumerator *blockenum, const char *file_path);

static json_value *access_json_hash(const json_value *p, const json_char *key) {
	if (p->type != json_object) return NULL;
	for (unsigned i=0; i < p->u.object.length; i++) {
		if (strcmp(p->u.object.values[i].name, key)==0) {
			return p->u.object.values[i].value;
		}
	}
	return NULL;
}

#define reterror(err) {fprintf(stderr, "%s\n", err);return;}
static bool assertinteger(const char *varname, const json_value *val) {
	if (val->type != json_integer) {
		fprintf(stderr, "Expected integer for parameter %s\n", varname);
		return false;
	}
	return true;
}
static bool try_assign_integer_variable(unsigned short *out, const char *varname, const char *key, const json_value *val) {
	if (strcmp(key, varname)==0) {
		if (assertinteger(varname, val)) {*out = val->u.integer;return true;}
		else return false;
	}
	return true;
}
static char *json_string_to_cstring(const json_value *val) {
	if (val->type != json_string) return NULL;
	const char *p = val->u.string.ptr;
	size_t     ln = val->u.string.length;
	char *res = (char*)malloc(ln+1);
	if (!res) return NULL;
	memcpy(res, p, ln);
	res[ln]=0;
	return res;
}
static void enum_map(BlockEnumerator *blockenum, const char *json_path) {
	unsigned long file_size;
	char *json = (char*)load_file(json_path, &file_size);
	json_value *p = json_parse(json, file_size);
	if (!p) reterror("Failed to parse json");
	
	if (p->type != json_object) reterror("JSON must be an associative array!");
	
	const json_value *map_type = access_json_hash(p, "type");
	if (!map_type) {
		reterror("JSON is not a JSON map (no type field)!");
	}
	if (!(map_type->type == json_string && strcmp(map_type->u.string.ptr, parameters.map_magic())==0)) {
		reterror("JSON is not a JSON map (type field is not ktank-map)!");
	}
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
	free(json);
}
static json_value *json_load(const char *json_path) {
	unsigned long file_size;
	char *json = (char*)load_file(json_path, &file_size);
	json_value *p = json_parse(json, file_size);
	free(json);
	if (!p) {
		fprintf(stderr, "Failed to parse json\n");
		return NULL;
	}
	return p;
}
static bool json_check_type(const json_value *p, const char *type_name) {
	if (p->type != json_object) {
		fprintf(stderr, "JSON must be an associative array!");
		return false;
	}
	
	const json_value *map_type = access_json_hash(p, "type");
	if (!map_type) {
		fprintf(stderr, "JSON is not a %s (no type field)!", type_name);
		return false;
	}
	if (!(map_type->type == json_string && strcmp(map_type->u.string.ptr, type_name)==0)) {
		fprintf(stderr, "JSON is not a %s (type field is not %s)!", type_name, type_name);
		return false;
	}
	return true;
}
class KeymapEnumerator
{
	public:
	ControllerDefinitions *controllers;
	void enumerate(const char *control, const char *command);
};
static bool enum_keymap(KeymapEnumerator *kmenum, const char *json_path) {
	json_value *p;
	if (!(p=json_load(json_path))) {
		return false;
	}
	if (!json_check_type(p, parameters.keymap_magic())) {
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
	BlockEnumerator blockenum(engine);
	enum_map(&blockenum, file_path);
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
