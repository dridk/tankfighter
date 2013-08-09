#include "misc.h"
#include <stdio.h>

using namespace sf;

std::string getDefaultFontName(void) {
#ifdef _WIN32
	return "c:\\windows\\fonts\\Arial.ttf";
#else
	return "/usr/share/fonts/truetype/droid/DroidSans.ttf";
#endif
}

std::string LowerCaseString(const std::string &str) {
	std::string data = str;
	std::transform(data.begin(), data.end(), data.begin(), tolower);
	return data;
}
double get_random(void) {
	return double(rand())/RAND_MAX;
}
double get_random(double max) {
	return get_random()*max;
}
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
	if (fread(file_contents, sizeof(char), input_file_size, input_file)<1) {
		fclose(input_file);
		free(file_contents);
		return NULL;
	}
	fclose(input_file);
	return file_contents;
}

void load_sprite(Sprite &sprite, Texture &tex, const char *path)
{
	if (!tex.loadFromFile(path)) {
		fprintf(stderr, "Failed to load %s\n", path);
		exit(1);
	}
	tex.setSmooth(true);
	sprite.setTexture(tex);
	FloatRect r=sprite.getLocalBounds();
	sprite.setOrigin(Vector2f(r.width/2, r.height/2));
}
void load_texture(Sprite &sprite, Texture &tex, const char *path) {
	load_sprite(sprite, tex, path);
	sprite.setOrigin(0,0);
	tex.setRepeated(true);
}
