#ifndef __KEYS_H__
#define __KEYS_H__
#include <SFML/Window/Keyboard.hpp>

struct NamedKey {
	const char *name;
	sf::Keyboard::Key key;
};

extern NamedKey named_keys[];
#endif
