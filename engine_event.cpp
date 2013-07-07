#include "engine_event.h"
#include "geometry.h"

EngineEvent::EngineEvent() {}
EngineEvent::~EngineEvent() {}
CollisionEvent::CollisionEvent() {
	interaction = IT_GHOST;
	first = NULL; second = NULL;
	retry = false;
}
