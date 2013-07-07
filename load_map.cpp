#include "load_map.h"
#include "misc.h"
#include "json.h"
#include "wall.h"
#include <stdio.h>
#include <stdlib.h>

struct Block {
	unsigned short x,y,width,height;
	char *texture_name;
};
class BlockEnumerator {
	public:
	BlockEnumerator(Engine *engine);
	void enumerate(const Block &block);
	private:
	Engine *engine;
};
static void enum_map(BlockEnumerator *blockenum, const char *file_path);

static json_value *access_json_hash(json_value *p, const json_char *key) {
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
		block.texture_name = NULL;
		for(int j=0; j < entity->u.object.length; j++) {
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
	Wall *wall = new Wall(block.x, block.y, block.width, block.height, block.texture_name, engine);
	engine->add(wall);
}

