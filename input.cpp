#include <SFML/Window/Window.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

using namespace sf;

static bool is_focused=true;

void treatLocalKeyEvent(const sf::Event &e) {
	if (e.type == Event::LostFocus) is_focused = false;
	else if (e.type == Event::GainedFocus) is_focused = true;
}
bool isLocalKeyPressed(Keyboard::Key key) {
	return is_focused && Keyboard::isKeyPressed(key);
}
