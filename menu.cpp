#include "menu.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Mouse.hpp>
#include "engine.h"

using namespace sf;

Menu::~Menu() {
}
size_t Menu::getItemCount(void) const {return items.size();}
Menu::Menu(Engine *engine):Entity(SHAPE_RECTANGLE, engine) {
	font.loadFromFile("/usr/share/fonts/truetype/droid/DroidSans.ttf");
	selectedItem = -1;
	irep = 0;
	validated = false;
	ompos = Mouse::getPosition(getEngine()->getWindow());
	mouseLeftWasPressed = true;
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
Menu::MenuMetrics Menu::getMetrics(void) const {
	MenuMetrics m;
	if (items.size() == 0) return m;
	m.hmargin = 200; m.wmargin = 200;
	m.tmargin = 20;
	m.hspacing = 1.5;
	unsigned hmargin = m.hmargin, wmargin = m.wmargin;
	Vector2u wsize = getEngine()->getWindow().getSize();
	m.wsize = wsize;
	int font_size = (wsize.y-hmargin)/((items.size() <= 3 ? 3 : items.size())+3)/m.hspacing;
	if (font_size < 12) font_size = 12;
	
	Vector2d size = text_size(font_size);
	size.y *= m.hspacing;
	if (size.x >= (wsize.x-wmargin)) {
		font_size = int(((double)font_size)/size.x*(wsize.x-wmargin));
		size = text_size(font_size);
		size.y *= m.hspacing;
	}
	if (font_size < 12) font_size = 12;

	m.font_size = font_size;

	m.texth = size.y/(items.size());
	m.start = 0; m.end = items.size();
	if (size.y >= (wsize.y-hmargin)) { /* too many items to fit in a screen */
		int dispCount = (wsize.y-hmargin)/m.texth;
		m.start = selectedItem - dispCount/2;
		if (m.start < 0) m.start = 0;
		m.end = m.start + dispCount;
		if (m.end >= int(items.size())) {
			m.end = items.size();
			m.start = m.end - dispCount;
		}
		size.y = dispCount * m.texth;
	}
	m.clr.left = (wsize.x - size.x)/2;
	m.clr.top = (wsize.y - size.y)/2;
	m.clr.width = size.x;
	m.clr.height = size.y;
	return m;
}
int Menu::getItemFromPosition(const Vector2f &pos) const {
	MenuMetrics m = getMetrics();
	if (pos.x < m.clr.left || pos.x >= m.clr.left+m.clr.width) return -1;
	if (pos.y < m.clr.top  || pos.y >= m.clr.top+m.clr.height) return -1;
	int item = m.start+int((pos.y-m.clr.top)/m.texth);
	if (item >= m.end || item < m.start) return -1;
	return item;
}
void Menu::draw(sf::RenderTarget &target) const {
	if (items.size() == 0) return;
	MenuMetrics m = getMetrics();
	RectangleShape r;
	r.setPosition(m.clr.left-m.tmargin/2, m.clr.top-m.tmargin/2);
	r.setSize(Vector2f(m.clr.width+m.tmargin, m.clr.height+m.tmargin));
	r.setFillColor(Color(128,128,128,160));
	target.draw(r);

	for(int i=m.start; i < m.end; ++i) {
		Vector2d pos = Vector2d(m.clr.left, m.clr.top + m.texth * (i-m.start));
		Color color;
		if (i == selectedItem) color = Color(192,0,0); else color = Color(255,255,255);
		draw_string(target, items[i].title.c_str(), pos, m.font_size, color);
	}
}
void Menu::event_received(EngineEvent *event) {
}

void Menu::addItem(const char *item_string, void *user_data, int beforeItem) {
	MenuItem item;
	item.title = item_string;
	item.user_data = user_data;
	if (beforeItem == -1) items.push_back(item);
	else items.insert(items.begin()+beforeItem, item);
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

bool altKeyPressed(void) {
	static Keyboard::Key keys[]={
		Keyboard::LShift,Keyboard::LAlt,Keyboard::LSystem,Keyboard::LControl,
		Keyboard::RShift,Keyboard::RAlt,Keyboard::RSystem,Keyboard::RControl
	};
	for(size_t i=0; i < sizeof(keys)/sizeof(keys[0]);i++) {
		if (Keyboard::isKeyPressed(keys[i])) return true;
	}
	return false;
}
void Menu::controllerFeedback(void) {
	if (validated) return;
	bool up = Keyboard::isKeyPressed(Keyboard::Up);
	bool down = Keyboard::isKeyPressed(Keyboard::Down);
	bool start = Keyboard::isKeyPressed(Keyboard::Home);
	bool end = Keyboard::isKeyPressed(Keyboard::End);
	bool pgup = Keyboard::isKeyPressed(Keyboard::PageUp);
	bool pgdown = Keyboard::isKeyPressed(Keyboard::PageDown);
	int alljoy=0;
	bool alljoybut=0;
	for(unsigned i=0; i < Joystick::Count; i++) {
		if (!Joystick::isConnected(i)) continue;
		float ay = Joystick::getAxisPosition(i, Joystick::Y);
		float dy = Joystick::getAxisPosition(i, Joystick::PovY);
		alljoy += int((ay+dy)*1.2/100);
		for(unsigned b=0; b < Joystick::getButtonCount(i); b++) {
			alljoybut |= Joystick::isButtonPressed(i, b);
		}
	}
	if (alljoy < 0) up=true;
	if (alljoy > 0) down=true;
	if (Keyboard::isKeyPressed(Keyboard::Return)) {
		validated = true;
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
	Vector2i mpos = Mouse::getPosition(getEngine()->getWindow());
	if (mpos != ompos) {
		int item = getItemFromPosition(Vector2f(mpos.x, mpos.y));
		if (item >= 0) selectByIndex(item);
		ompos = mpos;
	}
	if (Mouse::isButtonPressed(Mouse::Left) && !mouseLeftWasPressed && !altKeyPressed()) {
		int item = getItemFromPosition(Vector2f(mpos.x, mpos.y));
		if (item >= 0) {selectByIndex(item);validated=true;}
	}
	mouseLeftWasPressed = Mouse::isButtonPressed(Mouse::Left);
	if (alljoybut) validated = true;
}
bool Menu::selectionValidated(void) const {
	return validated;
}
void Menu::escape(void) {
	selectedItem = -1;
	validated = true;
}
