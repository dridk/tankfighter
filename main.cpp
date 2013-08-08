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

using namespace sf;
using namespace std;

#undef DEBUG_OBJECT_OUTLINES

int repl(void) {
	Engine engine;
	engine.loadMap("map2.json");
	engine.play();
	return 0;
}
int main(int argc, char **argv) {
	/* cmdline example: --joystick-player <joyid> --keyb-player <keyb-map-id> */
	void test_geometry_cpp();
	test_geometry_cpp();
	srand(time(NULL));
	return repl();
}
