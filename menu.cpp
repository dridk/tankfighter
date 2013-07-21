#include "menu.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Window/Keyboard.hpp>
#include "engine.h"

using namespace sf;

Menu::~Menu() {
}
Menu::Menu(Engine *engine):Entity(SHAPE_RECTANGLE, engine) {
	font.loadFromFile("/usr/share/fonts/truetype/droid/DroidSans.ttf");
	selectedItem = -1;
	irep = 0;
	validated = false;
}
Vector2d Menu::getSize() const {
	return Vector2d(0,0);
}
Vector2d Menu::movement(sf::Int64 tm) {
	controllerFeedback();
	return Vector2d(0,0);
}

void Menu::draw_string(RenderTarget &target, const char *str, Vector2d pos, int font_size, Color color) const {
	Text text;
	text.setCharacterSize(font_size);
	text.setString(str);
	text.setColor(color);
	text.setPosition(Vector2f(pos.x, pos.y));
	text.setFont(font);

	target.draw(text);
}
Vector2d Menu::text_size(unsigned font_size) const {
	Vector2d sz(0,0);
	for(size_t i=0; i < items.size(); ++i) {
		Text text;
		text.setCharacterSize(font_size);
		text.setString(items[i].title);
		text.setFont(font);
		FloatRect r = text.getLocalBounds();
		if (r.width  >= sz.x) sz.x = r.width;
		sz.y += r.height;
	}
	return sz;
}
void Menu::draw(sf::RenderTarget &target) const {
	if (items.size() == 0) return;
	unsigned hmargin = 200, wmargin = 200;
	unsigned tmargin = 20;
	double hspacing = 1.5;
	int start, end;
	Vector2u wsize = getEngine()->getWindow().getSize();
	int font_size = (wsize.y-hmargin)/((items.size() <= 3 ? 3 : items.size())+3)/hspacing;
	if (font_size < 12) font_size = 12;
	
	Vector2d size = text_size(font_size);
	size.y *= hspacing;
	if (size.x >= (wsize.x-wmargin)) {
		font_size = int(((double)font_size)/size.x*(wsize.x-wmargin));
		size = text_size(font_size);
		size.y *= hspacing;
	}
	if (font_size < 12) font_size = 12;

	double texth = size.y/(items.size());
	start = 0; end = items.size();
	if (size.y >= (wsize.y-hmargin)) { /* too many items to fit in a screen */
		int dispCount = (wsize.y-hmargin)/texth;
		start = selectedItem - dispCount/2;
		if (start < 0) start = 0;
		end = start + dispCount;
		if (end >= int(items.size())) {
			end = items.size();
			start = end - dispCount;
		}
		size.y = dispCount * texth;
	}
	Vector2d pos0;
	pos0.x = (wsize.x - size.x)/2;
	pos0.y = (wsize.y - size.y)/2;
	RectangleShape r;
	r.setSize(Vector2f(size.x+tmargin, size.y+tmargin));
	r.setPosition(pos0.x-tmargin/2, pos0.y-tmargin/2);
	r.setFillColor(Color(128,128,128,160));
	target.draw(r);

	for(int i=start; i < end; ++i) {
		Vector2d pos = Vector2d(pos0.x, pos0.y + texth * (i-start));
		Color color;
		if (i == selectedItem) color = Color(192,0,0); else color = Color(255,255,255);
		draw_string(target, items[i].title.c_str(), pos, font_size, color);
	}
}
void Menu::event_received(EngineEvent *event) {
}

void Menu::addItem(const char *item_string, void *user_data) {
	MenuItem item;
	item.title = item_string;
	item.user_data = user_data;
	items.push_back(item);
	if (selectedItem < 0 && items.size() == 1) selectedItem = 0;
}

int Menu::getSelected(void **puser_data) const {
	void *user_data;
	if (selectedItem < 0) user_data = NULL; else user_data = items[selectedItem].user_data;
	if (puser_data) *puser_data = user_data;
	return selectedItem;
}
void Menu::selectNext(void) {
	if (items.size() == 0) return;
	selectedItem = (selectedItem+1)%items.size();
}
void Menu::selectPrevious(void) {
	if (items.size() == 0) return;
	selectedItem --;
	if (selectedItem < 0) selectedItem = items.size() - 1;
}
void Menu::selectByIndex(int itemIndex) {
	if (items.size() == 0) return;
	selectedItem = itemIndex;
	if (selectedItem >= 0) selectedItem %= items.size();
	if (selectedItem <  0) selectedItem = items.size() - ((-selectedItem) % items.size());
}
void Menu::selectFirst(void) {
	if (items.size() == 0) return;
	selectedItem = 0;
}
void Menu::selectLast(void) {
	if (items.size() == 0) return;
	selectedItem = items.size() - 1;
}

void Menu::controllerFeedback(void) {
	if (validated) return;
	bool up = Keyboard::isKeyPressed(Keyboard::Up);
	bool down = Keyboard::isKeyPressed(Keyboard::Down);
	bool start = Keyboard::isKeyPressed(Keyboard::Home);
	bool end = Keyboard::isKeyPressed(Keyboard::End);
	bool pgup = Keyboard::isKeyPressed(Keyboard::PageUp);
	bool pgdown = Keyboard::isKeyPressed(Keyboard::PageDown);
	if (Keyboard::isKeyPressed(Keyboard::Return)) {
		validated = true;
		return;
	} else if (Keyboard::isKeyPressed(Keyboard::Escape)) {
		validated = true;
		selectedItem = -1;
		return;
	}
	if (!(up || down || start || end || pgup || pgdown))
		{key_repeat.restart();irep = 0;}
	else if (irep == 0 || key_repeat.getElapsedTime().asMilliseconds() >= (irep == 1 ? 350 : 100)) {
		if (up) selectPrevious();
		if (down) selectNext();
		if (pgup) selectByIndex(selectedItem-10);
		if (pgdown) selectByIndex(selectedItem+10);
		if (start) selectFirst();
		if (end) selectLast();
		irep ++;
		if (irep >= 10) irep = 10;
		key_repeat.restart();
	}
}
bool Menu::selectionValidated(void) const {
	return validated;
}
