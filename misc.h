#ifndef __MISC_H__
#define __MISC_H__
#include "coretypes.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <string>
#include <sstream>
#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626338
#endif

void load_sprite(sf::Sprite &sprite, sf::Texture &tex, const char *path);
void load_texture(sf::Sprite &sprite, sf::Texture &tex, const char *path);

void *load_file(const char *input_file_name, unsigned long *file_size);

double get_random(void);
double get_random(double max);
std::string LowerCaseString(const std::string &str);

std::string getDefaultFontPath(void);
bool string2long(const char *s, long *v);
bool string2dbl(const char *s, double *v);

template <class T>
std::string tostring(const T &x) {
	std::ostringstream o;
	o << x;
	return o.str();
}
#endif
