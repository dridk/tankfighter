#ifndef __LOAD_MAP_H__
#define __LOAD_MAP_H__
#include "coretypes.h"
#include <vector>


class Engine;

void load_map(Engine *engine, const char *file_path);

class KeymapController;

struct ControllerDefinitions {
	std::vector<KeymapController*> forplayer; /* index 0  = default */
	ControllerDefinitions();
	~ControllerDefinitions();
};
void load_keymap(ControllerDefinitions &controllers, const char *file_path);
#endif
