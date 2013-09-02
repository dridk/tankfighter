#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Audio.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "engine.h"
#include "parameters.h"
#include <unistd.h>
#include <limits.h>


using namespace sf;
using namespace std;

#undef DEBUG_OBJECT_OUTLINES
#define stringify(path) path

static void set_data_directory() {
#ifndef _WIN32
	const char *testfile = "sprites";
	char buffer[256];
	char exepath[PATH_MAX+1];
	if (access(testfile, R_OK)!=-1) return;
	sprintf(buffer, "/proc/%lu/exe", (unsigned long)getpid());
	if (readlink(buffer, exepath, PATH_MAX+1) != -1) {
		/* we got an exe path on Linux */
		char *e = strrchr(exepath, '/');
		if (e && sizeof(exepath) > (e+1-exepath)+strlen(testfile)+1) {
			memmove(e+1, testfile, strlen(testfile)+1);
			if (access(exepath, R_OK)!=-1) {
				*e = 0;
				chdir(exepath);
				return;
			}
		}
	}
	/* CWD is not a data directory */
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
