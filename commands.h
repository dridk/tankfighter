#ifndef __INCL_COMMANDS_H__
#define __INCL_COMMANDS_H__
#include "controller.h"
#include <vector>
#include <SFML/Window.hpp>

enum TriggerType {TT_JoystickAxisNeg, TT_JoystickAxisPos, TT_JoystickButton, TT_KeyboardKey, TT_MouseButton, TT_MousePosition};
struct Trigger {
	TriggerType type;
	int keycode;
};
enum CommandPiece {CP_Tank_Orientation, CP_Tank_Position, CP_Canon_Orientation, CP_Shooter};
enum CommandDirection {CD_Vertical, CD_Horizontal, CD_Axial, CD_Lateral, CD_Rotate};
enum CommandSens {CD_Positive, CD_Negative};
enum CommandRelativity {CR_Absolute, CR_Relative, CR_LookAt}; /* CR_Absolute is useful for tank & canon angles */
struct Command {
	CommandPiece piece;
	CommandDirection axis;
	CommandSens sens;
	CommandRelativity relativity; /* CR_Absolute is useful for tank & canon angles */
	/* CR_lookat is used with CP_Tank_Orientation to make canon or tank look at an absolute screen position (mouse cursor). It needs two control axes at once. */
};
struct TriggerCommand {
	Trigger trigger;
	Command command;
};
/*
  Command string example:
  [tank]|canon turn left|right
  [tank]|canon direction down|up|left|right
  [tank] move forward|backward|latleft|latright|left|right|up|down
  [tank]|canon lookat
  shoot
*/


class KeymapController: public LocalController
{
	public:
	KeymapController(int joyid = -1);
	KeymapController *clone(int joyid);
	void mapControl(const char *trigger, const char *command);
	int getJoystickId(void) const;
	virtual void reportPlayerMovement(Player *player, PlayerControllingData &pcd);
	static bool maybeConcerned(const sf::Event &e);
	virtual bool isConcerned(const sf::Event &e) const;
	bool isConcernedAndAffects(const sf::Event &e, int &ojoyid) const;

	private:
	typedef std::vector<TriggerCommand>::iterator iterator;
	std::vector<TriggerCommand> commandmap;
	signed char joyid;

};
#endif
