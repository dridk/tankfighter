#ifndef __WALL_H__
#define __WALL_H__
#include "entity.h"
#include <string>

class Wall: public Entity
{
	public:
	Wall(double x, double y, double w, double h, const char *texture_name);
	virtual ~Wall();
	virtual Vector2d getSize() const;

	virtual void draw(sf::RenderTarget &target) const;
	virtual Vector2d movement(sf::Int64 tm);
	virtual void event_received(EngineEvent *event);

	private:
	sf::Vector2f size;
	std::string texture_name;
};
#endif
