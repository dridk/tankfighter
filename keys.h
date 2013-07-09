#ifndef __KEYS_H__
#define __KEYS_H__
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Joystick.hpp>
#include <stddef.h>

struct NamedKey {
	const char *name;
	unsigned short keycode;
};

extern NamedKey key_codemap[];
extern NamedKey mouse_codemap[];
extern NamedKey joyaxis_codemap[];
extern NamedKey joybutton_codemap[];
#endif
