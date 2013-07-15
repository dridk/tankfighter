#ifndef __WALL_H__
#define __WALL_H__
#include "entity.h"
#include <string>

class Engine;

class Wall: public Entity
{
	public:
	Wall(double x, double y, double w, double h, const char *texture_name, Engine *engine);
	virtual ~Wall();
	virtual Vector2d getSize() const;

	virtual void draw(sf::RenderTarget &target) const;
	virtual Vector2d movement(sf::Int64 tm);
	virtual void event_received(EngineEvent *event);
	std::string getTextureName(void) const;

	private:
	sf::Vector2f size;
	std::string texture_name;
};
#endif
