#include "coretypes.h"
#include "messages.h"
#include "engine.h"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Color.hpp>
#include "misc.h"

using namespace sf;

Messages::Messages(Engine *engine):Entity(SHAPE_EMPTY, engine) {
	font.loadFromFile(getDefaultFontPath().c_str());
}
Messages::~Messages() {
}
Vector2d Messages::getSize() const {
	return Vector2d(0,0);
}
Vector2d Messages::movement(sf::Int64 tm) {
	for(size_t i=0; i < messages.size();) {
		if (messages[i].time.getElapsedTime().asMilliseconds() >= 3000) {
			messages.erase(messages.begin()+i);
		} else i++;
	}
	return Vector2d(0,0);
}
void Messages::event_received(EngineEvent *event) {}

void Messages::display(const std::string &text, const sf::Color *pc) {
	Color c(0,0,0);
	if (pc) c=*pc;
	Message m;
	m.msg = text;
	m.color = c;
	messages.push_back(m);
}
void Messages::draw_string(RenderTarget &target, Vector2d pos, const Message &msg) const {
	Text text;
	text.setCharacterSize(24);
	text.setString(msg.msg);
	text.setColor(msg.color);
	text.setPosition(Vector2f(pos.x, pos.y));
	text.setFont(font);

	target.draw(text);
}
void Messages::draw(sf::RenderTarget &target) const {
	if (messages.size() == 0) return;
	double hinterval=1.5;
	double hmargin = 10, wmargin = 10;
	RectangleShape r;
	Vector2u wsize = getEngine()->getWindow().getSize();
	r.setFillColor(Color(128,128,128,128));
	r.setSize(Vector2f(wsize.x, hmargin + messages.size()*(24*hinterval)));
	target.draw(r);

	Vector2d pos(hmargin/2,wmargin/2);
	for(size_t i=0; i < messages.size();i++) {
		draw_string(target, Vector2d(pos.x, pos.y + i*(24*hinterval)), messages[i]);
	}
}

