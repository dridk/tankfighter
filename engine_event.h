#ifndef __ENGINE_EVENT_H__
#define __ENGINE_EVENT_H__
#include "geometry.h"

class Entity;

enum EngineEventType {
	NULL_EVENT,
	ENTITY_CREATED_EVENT,
	ENTITY_DESTROYED_EVENT,
	COLLIDE_EVENT,
	COMPLETED_MOVEMENT_EVENT
};

struct EngineEvent
{
	EngineEventType type;
	EngineEvent();
	virtual ~EngineEvent();
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
	Entity *first;  /* in */
	Entity *second; /* in */
	InteractionType interaction; /* out */
};
struct CompletedMovementEvent: EngineEvent
{
	Entity *entity;
	Vector2d position;
	Vector2d new_speed;
	bool has_new_speed;
};
#endif
