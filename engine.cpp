#include "engine.h"

void Engine::add(Entity *entity) {
	entities.push_back(entity);
}
void Engine::destroy(Entity *entity) { /* Removes entity from engine and deletes the underlying object */
	entity->setKilled();
}
void Engine::broadcast(EngineEvent *event) {
	for(EntitiesIterator it=entities.begin(); it != entities.end(); ++it) {
		(*it)->event_received(event);
	}
}
bool Engine::step(void) {
	draw();
	compute_physics();
	destroy_flagged();
	return must_quit;
}
void Engine::draw(void) {
	for(EntitiesIterator it=entities.begin(); it != entities.end(); ++it) {
		(*it)->draw();
	}
}
static void distanceToInterval(double x, double left, double right) {
	if (x <= left) return left-x;
	if (x >= right) return x-right;
	return 0; /* inside interval */
}
static int positionToInterval(double x, double left, double right) {
	if (x <= left) return -1;
	if (x >= right) return 1;
	return 0;
}
static double pointsDistance(Vector2f p1, Vector2f p2) {
	Vector2f v=p2-p1;
	return sqrt(v.x*v.x + v.y*v.y);
}
static double distanceToRectangle(Vector2f pt, const FloatRect &br) {
	double x = pt.x, y = pt.y;
	double distance=0;
	Vector2f rel;
	Vector2f vertices[4];
	for(int i=0; i < 4; i++) {
		if (i%2 == 0) vertices[i].x=br.left; else vertices[i].x=br.left+br.width;
		if (i/2 == 0) vertices[i].y=br.top;  else vertices[i].y=br.top +br.height;
	}
	int yPos = positionToInterval(y, br.top, br.top+br.height);
	int xPos = positionToInterval(x, br.left, br.left+br.width);
	if (xPos*yPos != 0) {/* we are next to a vertice */
		xPos=(xPos+1)/2; /* 0 on left, 1 on right */
		yPos=(yPos+1)/2; /* 0 on top, 1 on bottom */
		distance = pointsDistance(Vector2f(x,y), vertices[xPos + yPos*2]);
	} else if (xPos == 0) { /* we are next to top or bottom side */
		distance = distanceToInterval(y, br.top, br.top+br.height);
	} else { /* we are next to left or right side */
		distance = distanceToInterval(x, br.left, br.left+br.width);
	}
}
static void intersects(Entity *a, Entity *b) {
	if (a->shape == SHAPE_CIRCLE && b->shape == SHAPE_CIRCLE) {
		double distance = pointsDistance(a->position, b->position);
		return distance < (a.getSize().x+b.getSize().x)/2;
	} else if (a->shape == SHAPE_CIRCLE && b->shape == SHAPE_RECTANGLE) {
		double distance = distanceToRectangle(a->position, b->getBoundingRect());
		return distance < (a.getSize().x+b.getSize().x)/2;
	} else if (a->shape == SHAPE_RECTANGLE && b->shape == SHAPE_CIRCLE) {
		return intersects(b,a);
	}
}
void Engine::compute_physics(void) {
	for(EntitiesIterator it=entities.begin(); it != entities.end(); ++it) {
		Entity *entity = (*it);
		Vector2f movement = entity->movement();
		for (EntitiesIterator itc=entities.begin(); itc != entities.end(); ++itc) {
			Entity *centity = *itc;
			if (centity == entity) continue;
			if (intersects(centity, entity)) {}
		}
	}
}
void Engine::destroy_flagged(void);
