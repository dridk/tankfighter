#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Audio.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <math.h>
#include "json.h"
#include "geometry.h"
#include "misc.h"
#include "engine.h"
#include "load_map.h"
#include "player.h"
#include "controller.h"
#include "commands.h"
#include "parameters.h"


using namespace sf;
using namespace std;

#undef DEBUG_OBJECT_OUTLINES
#define stringify(path) path

static void set_data_directory() {
#ifndef _WIN32
	chdir(stringify(TF_PREFIX) "/share/" stringify(TF_NAME) "-" stringify(TF_VERSION));
#endif
}
int main(int argc, char **argv) {
	set_data_directory();
	srand(time(NULL));
	if (!parameters.parseCmdline(argc, argv)) return 0;
	Engine engine;
	engine.loadMap(parameters.map().c_str());
	engine.play();
	return 0;
}
