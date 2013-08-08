#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__
#include <math.h>

class Parameters
{
	/* class defining all constants */
	public:
	/* constants for networking */
	unsigned UDP_MTU(void) {return 512;}
	unsigned C2S_Packet_interval_US(void) {return 10000;}
	unsigned short serverPort(void) {return 1330;}
	unsigned short clientPort(void) {return 1329;}
	int resendPacketAfterMS(void) {return 500;}
	int connectionTimeoutInSecs(void) {return 10;}
	int disconnectionTimeoutMS(void) {return 2000;}
	int clientsCleanupIntervalMS(void) {return 1000;}
	int udpPortRange(void) {return 100;}
	int maxDupPacketTimeSecs(void) {return 10;}
	/* constants for game rules */
	float missileDelayMS(void) {return 200;} /* how often a missile is launched */
	float tankDiameter(void) {return 128;}
	const char *tankSpriteName(void) {return "car";}
	const char *canonSpriteName(void) {return "canon";}
	double canon_rotation_speed(void) {
		return 3e-4/180*M_PI; /* radians per microsecond */
	}
	double tank_rotation_speed(void) {return 3e-4/180*M_PI;}
	double linear_tank_speed(void) {return 3e-4;} /* px per usec */

	double maxMissileLifeDurationMS(void) {return 2000;}
	double missileSpeed(void) {return 9e-4;} /* px/usec */
	double missileDiameter(void) {return 16;}

	/* constants for game engine */
	unsigned minFPS(void) {return 15;}
	unsigned maxFPS(void) {return 60;}

	/* geometry */
	double minWallDistance(void) {return 1e-3;} /* in pixels */

	/* paths for game data */
	const char *spritesDirectory(void) {return "sprites/";}
	const char *spritesExtension(void) {return ".png";}
	const char *keymap_magic(void) {return "ktank-ctrl-map";}
	const char *map_magic(void) {return "ktank-map";}

	/* controller constants */
	double joyTankSpeedCalibration() {return 0.3;}
	double joyCanonDirectionCalibration() {return 0.15;}
	double joyDefaultCalibration() {return 0.2;}
	int serverDiscoveryTimeoutMS() {return 2000;}
	int serverDiscoveryPeriodMS() {return 500;}
};
extern Parameters parameters;
#endif
