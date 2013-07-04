#ifndef __ENGINE_EVENT_H__
#define __ENGINE_EVENT_H__

enum EngineEventType {
	NULL_EVENT,
	ENTITY_CREATED_EVENT,
	ENTITY_DESTROYED_EVENT,
	COLLIDE_EVENT
};

struct EngineEvent
{
	EngineEventType type;
};
struct EntityCreatedEvent: EngineEvent
{
	Entity *entity;
};
struct EntityDestroyedEvent: EngineEvent
{
	Entity *entity;
};
struct CollisionEvent: EngineEvent
{
	Entity *first;
	Entity *second;
};
#endif
