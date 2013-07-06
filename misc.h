#ifndef __MISC_H__
#define __MISC_H__
#include "coretypes.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

void load_sprite(sf::Sprite &sprite, sf::Texture &tex, const char *path);
void load_texture(sf::Sprite &sprite, sf::Texture &tex, const char *path);

void *load_file(const char *input_file_name, unsigned long *file_size);

double get_random(void);
double get_random(double max);
#endif
