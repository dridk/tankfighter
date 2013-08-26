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
#include "geometry.h"
#include <math.h>
#include <stdint.h>

struct Block {
	TFPolygon polygon;
	double angle;
	TextureDesc texture;
	bool isMapBoundaries;
};

std::string json_string_to_stdstring(const json_value *v) {
	std::string r;
	char *p = json_string_to_cstring(v);
	if (!p) return "";
	r = p;
	free(p);
	return r;
}
bool json2texturedesc(const json_value *entity, TextureDesc &desc) {
	desc.clear();
	if (entity->type == json_string) {
		desc.name = json_string_to_stdstring(entity);
		return true;
	}
	if (entity->type != json_object) return false;
	for(size_t j=0; j < entity->u.object.length; j++) {
		const char *key = entity->u.object.values[j].name;
		const json_value *value = entity->u.object.values[j].value;
		try_assign_float_variable(&desc.xscale, "xscale", key, value);
		try_assign_float_variable(&desc.yscale, "yscale", key, value);
		double scale=0;
		if (try_assign_double_variable(&scale, "scale", key, value)) {
			desc.xscale = desc.yscale = scale;
		}
		try_assign_float_variable(&desc.xoff, "xoff", key, value);
		try_assign_float_variable(&desc.yoff, "yoff", key, value);
		double angle=0;
		try_assign_double_variable(&angle, "angle", key, value);
		if (fabs(angle) > 1e-6) {
			desc.angle = M_PI/180*angle;
		}
		if (strcmp(key, "mapping")==0) {
		if (char *mapping = json_string_to_cstring(value)) {
			if (strcmp(mapping, "tile")==0) desc.mapping = MAPPING_TILE;
			else if (strcmp(mapping, "stretch")==0) desc.mapping = MAPPING_STRETCH;
			else if (strcmp(mapping, "absolute tile")==0) desc.mapping = MAPPING_TILE_ABSOLUTE;
			else {
				fprintf(stderr, "Unknown mapping type %s\n", mapping);
			}
			free(mapping);
		} else {
			fprintf(stderr, "Expected mapping type as JSON string\n");
		}
		}
		if (strcmp(key, "image")==0) {
			desc.name = json_string_to_stdstring(value);
		}
	}
	return true;
}

class BlockEnumerator {
	public:
	BlockEnumerator(Engine *engine);
	void enumerate(const Block &block);
	private:
	Engine *engine;
};
static void enum_map(BlockEnumerator *blockenum, Vector2d &map_size, const char *file_path);

#define reterror(err) {fprintf(stderr, "%s\n", err);return;}

static bool read_json_polygon(TFPolygon &poly, const json_value *arr) {
	poly.clear();
	if (arr->type != json_array) return false;
	poly.reserve(arr->u.array.length);
	for(size_t i=0; i < arr->u.array.length; i++) {
		const json_value *point = arr->u.array.values[i];
		if (point->type != json_array) continue;
		if (point->u.array.length < 2) continue;
		json_value **values = point->u.array.values;
		Vector2d pt;
		if (!(json_assign_double(&pt.x,values[0]) && json_assign_double(&pt.y,values[1])))
			continue;
		poly.push_back(pt);
	}
	return true;
}
static void enum_map(BlockEnumerator *blockenum, Vector2d &map_size, const char *json_path) {
	json_value *p = json_parse_file(json_path, parameters.map_magic().c_str());
	if (!p) return;
	Block bgblock;

	const json_value *width = access_json_hash(p, "width");
	const json_value *height = access_json_hash(p, "height");
	if (!(width && width->type == json_integer)) {
		reterror("Map width must be specified as an integer");
	} else map_size.x = width->u.integer;
	if (!(height && height->type == json_integer)) {
		reterror("Map width must be specified as an integer");
	} else map_size.y = height->u.integer;
	
	bgblock.angle = 0;
	bgblock.isMapBoundaries = true;
	DoubleRect bgrect(0,0,0,0);
	bgrect.width = map_size.x;
	bgrect.height = map_size.y;
	Rectangle2Polygon(bgrect, bgblock.polygon);
	const json_value *bgtexture = access_json_hash(p, "background");
	if (bgtexture) {
		json2texturedesc(bgtexture, bgblock.texture);
	} else {
		bgblock.texture.name = parameters.defaultBackgroundTexture();
	}
	blockenum->enumerate(bgblock);
	
	const json_value *blocks = access_json_hash(p, "blocks");
	if (!blocks) reterror("Map lacks a blocks array!");
	if (!blocks->type == json_array) reterror("Map blocks field should be an array!");
	for(size_t i=0; i < blocks->u.array.length; i++) {
		const json_value *entity = blocks->u.array.values[i];
		if (entity->type != json_object) {
			fprintf(stderr, "Map block is not hash! Block ignored!");
			continue;
		}
		Block block;
		block.isMapBoundaries = false;
		block.angle = 0;
		bool is_polygon=false;
		DoubleRect rect(0,0,0,0);
		for(size_t j=0; j < entity->u.object.length; j++) {
			const char *key = entity->u.object.values[j].name;
			const json_value *value = entity->u.object.values[j].value;
			try_assign_double_variable(&rect.left, "x", key, value);
			try_assign_double_variable(&rect.top, "y", key, value);
			try_assign_double_variable(&rect.width, "w", key, value);
			try_assign_double_variable(&rect.height, "h", key, value);
			double degangle = 0;
			try_assign_double_variable(&degangle, "angle", key, value);
			if (fabs(degangle) >= 1e-4) {
				block.angle = degangle/180*M_PI;
			}
			if (strcmp(key, "texture")==0) {
				json2texturedesc(value, block.texture);
			}
			if (strcmp(key, "points")==0) {
				read_json_polygon(block.polygon, value);
			}
			if (strcmp(key, "shape")==0 && value->type == json_string
				&& value->u.string.length == 7 && strncmp(value->u.string.ptr, "polygon", 7)==0) {
				is_polygon = true;
			}
			if (strcmp(key, "background")==0) {
			}
		}
		if (!is_polygon) {
			Rectangle2Polygon(rect, block.polygon);
		}
		blockenum->enumerate(block);
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
	if (!(p=json_parse_file(json_path, parameters.keymap_magic().c_str()))) {
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

#ifdef DEBUG_KEYMAP
	fprintf(stderr, "map %s to %s for player %d\n", control, cmd.c_str(), idPlayer);
#endif
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
	Wall *wall = new Wall(block.polygon, block.angle, block.texture, engine);
	wall->isMapBoundaries(block.isMapBoundaries);
	engine->add(wall);
}

ControllerDefinitions::ControllerDefinitions() {}
ControllerDefinitions::~ControllerDefinitions() {
	for(unsigned i=0; i < forplayer.size(); i++) {
		if (forplayer[i]) delete forplayer[i];
	}
}
