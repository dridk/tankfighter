#ifndef __MESSAGES_H__
#define __MESSAGES_H__
#include "entity.h"
#include <deque>
#include <SFML/Graphics/Font.hpp>

class Messages:public Entity
{
	public:
	Messages(Engine *engine);
	~Messages();
	virtual Vector2d getSize() const ;
	virtual void draw(sf::RenderTarget &target) const;
	virtual Vector2d movement(sf::Int64 tm);
	virtual void event_received(EngineEvent *event);

	void display(const std::string &text, const sf::Color *c = NULL);
	private:
	struct Message {
		std::string msg;
		sf::Color color;
		sf::Clock time;
	};
	std::deque<Message> messages;
	sf::Font font;
	void draw_string(sf::RenderTarget &target, Vector2d pos, const Message &msg) const;
};
#endif
