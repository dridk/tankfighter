#ifndef __PLAYER_H__
#define __PLAYER_H__
#include "coretypes.h"
#include "entity.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System.hpp>
#include <SFML/System/Clock.hpp>

class Controller;
class Engine;

enum PCDFlags {PCD_Canon_Angle=1, PCD_Tank_Angle=2, PCD_Position=4, PCD_Score=8, PCD_Disconnect=16, PCD_Shoot=32, PCD_Preserve_Tank_Rotation=64, PCD_Adapt_Canon_Angle=128};
struct PlayerControllingData
{
	unsigned flags;
	float canon_angle, tank_angle; /* set absolute angle */
	float canon_rotation, tank_rotation; /* increase/decrease angle */
	Vector2d position, movement;
	int new_score;
	PlayerControllingData();

	void setCanonAngle(float angle);
	void rotateCanon(float angleSpeed);

	void preserveTankAngle(void);
	void adaptCanonAngle(void);
	void setTankAngle(float angle);
	void rotateTank (float angleSpeed);

	void move(Vector2d speed);
	void setPosition(Vector2d position);

	void keepShooting(void);
	void setScore(int score);
};

class Player: public Entity
{
	public:
	Player(Controller *controller, Engine *engine);
	~Player();
	virtual Vector2d getSize() const;
	virtual void draw(sf::RenderTarget &target) const;
	virtual Vector2d movement(sf::Int64 tm);
	virtual void event_received(EngineEvent *event);
	
	sf::Color getColor(void);
	double getCanonAngle(void);
	double getTankAngle(void);
	Vector2d getPosition(void);

#if 0
	void setCanonAngle(float angle);
	void rotateCanon(float angleSpeed);

	void preserveTankAngle(void);
	void adaptCanonAngle(void);
	void setTankAngle(float angle);
	void rotateTank (float angleSpeed);

	void move(Vector2d speed);
	void setPosition(Vector2d position);

	void keepShooting(void);
#endif
	void setScore(int score);
	int  getScore(void);

	void killedBy(Player *player);
	void killedPlayer(Player *player);

	Controller *getController(void);

	private:
	static const short maxMissileCount = 3;
	void teleport(void);
	Controller *controller;
	static const float missileDelay; /* milliseconds */
	float tank_direction;
	float canon_direction;
	sf::Color color;
	sf::Clock shoot_clock;

	int playerUID;
	static int UID;
	int score;
	short missileCount;

/* dynamic info */
	bool teleporting;
#if 0
	bool adapt_canon_angle;
	bool preserve_tank_angle;
	bool is_shooting;
	double canon_rotation, tank_rotation;
	Vector2d tank_movement;
	Vector2d tank_goto;
#endif
	void try_shoot(void);
	sf::Sprite &getSprite(const char *name) const;
	void computeRandomPosition(void);
};
#endif
