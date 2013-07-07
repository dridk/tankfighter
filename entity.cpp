#include "entity.h"


Entity::Entity(EntityShape shape0, Engine *engine0) {
	shape = shape0;
	position.x = 0; position.y = 0;
	isKilledFlag = false;
	engine = engine0;
}
Entity::~Entity() {
	engine = NULL;
}
