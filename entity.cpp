#include "entity.h"


Entity::Entity(EntityShape shape0) {
	shape = shape0;
	position.x = 0; position.y = 0;
	isKilledFlag = false;
	engine = NULL;
}
Entity::~Entity() {
	engine = NULL;
}
