#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__
#include <math.h>
#include <map>
#include <string>
#include <SFML/Window/VideoMode.hpp>
#include <ostream>
#include "json.h"

enum PType {PBoolean = json_boolean, PInteger = json_integer, PDouble = json_double, PString = json_string};
struct PVariable {
	PType		datatype;
	const char     *defval;
};
class Parameters
{
	/* class defining all constants */
	public:
	Parameters();
	long		getAsLong	(const char *name);
	double		getAsDouble	(const char *name);
	std::string	getAsString	(const char *name);
	bool		getAsBoolean	(const char *name);
	bool		set		(const char *name, const std::string &value);
	bool 		set		(const char *name, const json_value *value);
	bool		parseCmdline	(int argc, char **argv);
	bool		parseFile	(const char *config_file);
	void		outputHelpString(std::ostream &out);

	/* constants for networking */
	unsigned UDP_MTU(void);
	unsigned C2S_Packet_interval_US(void);
	unsigned short serverPort(void);
	unsigned short clientPort(void);
	double resendPacketAfterMS(void);
	double connectionTimeoutInSecs(void);
	double disconnectionTimeoutMS(void);
	double clientsCleanupIntervalMS(void);
	int udpPortRange(void);
	double maxDupPacketTimeSecs(void);
	/* constants for game rules */
	double missileDelayMS(void);
	double tankDiameter(void);
	std::string tankSpriteName(void);
	std::string canonSpriteName(void);
	std::string missileSpriteName(void);
	double canon_rotation_speed(void);
	double tank_rotation_speed(void);
	double linear_tank_speed(void);

	double maxMissileLifeDurationMS(void);
	double missileSpeed(void);
	double missileDiameter(void);

	/* constants for game engine */
	unsigned minFPS(void);
	unsigned maxFPS(void);

	/* geometry */
	double minWallDistance(void);

	/* paths for game data */
	std::string spritesDirectory(void);
	std::string spritesExtension(void);
	std::string keymap_magic(void);
	std::string map_magic(void);
	std::string config_magic(void);

	/* controller constants */
	double joyTankSpeedCalibration();
	double joyCanonDirectionCalibration();
	double joyDefaultCalibration();
	int serverDiscoveryTimeoutMS();
	int serverDiscoveryPeriodMS();

	std::string defaultFontName(void);
	bool fullscreen();
	std::string map();
	std::string keymap();
	std::string config();
	bool startServer();
	std::string joinAddress();
	long screenWidth();
	long screenHeight();
	sf::VideoMode getVideoMode();

	private:
	typedef std::map<std::string, std::string> Valmap;
	typedef std::map<std::string, PVariable> Varmap;
	typedef Varmap::value_type VarmapVal;
	typedef Varmap::iterator VarmapIterator;
	typedef Valmap::value_type ValmapVal;
	typedef Valmap::iterator ValmapIterator;
	Valmap values;
	Varmap variables;
};
extern Parameters parameters;
#endif
