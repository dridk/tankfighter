#ifndef __INPUT_H__
#define __INPUT_H__
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window.hpp>

bool isLocalKeyPressed(sf::Keyboard::Key key);
void treatLocalKeyEvent(const sf::Event &e);
#endif
