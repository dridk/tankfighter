#include "parameters.h"
#include <stdio.h>
#include <string.h>

Parameters parameters;

static struct ParameterDef {
	const char 	*name;
	PType		datatype;
	const char     *defval;
	char		cmdline;
} pdefs[]={
/* network */
	 {"udpMTU", PInteger, "512"}
	,{"NetworkFPS", PDouble, "60"}
	,{"serverPort", PInteger, "1330"}
	,{"clientPort", PInteger, "1320"}
	,{"connectionTimeout", PDouble, "10000"}
	,{"disconnectionTimeout", PDouble, "2000"}
	,{"broadcastPeriod", PDouble, "500"}
	,{"udpPortRange", PInteger, "100"}

/* game */
	,{"missileDelay", PDouble, "200"}
	,{"tankDiameter", PDouble, "128"}
	,{"tankSpriteName", PString, "car"}
	,{"canonSpriteName", PString, "canon"}
	,{"missileSpriteName", PString, "bullet"}
	,{"canonRotationSpeed", PDouble, "0.00523"} /* rad/ms */
	,{"tankRotationSpeed", PDouble, "0.00523"}
	,{"linearTankSpeed", PDouble, "0.3"} /* px per millisecond */
	,{"missileDuration", PDouble, "2000"}
	,{"missileSpeed", PDouble, "0.9"}
	,{"missileDiameter", PDouble, "16"}
	,{"minFPS", PDouble, "15"}
	,{"maxFPS", PDouble, "60"}

/* file system */
	,{"spritesDirectory", PString, "sprites/"}
	,{"spritesExtension", PString, ".png"}
	,{"defaultFontName", PString, "sans-serif:slant=roman:weight=normal:lang=en_US:scalable=true"}

	,{"fullscreen", PBoolean, "1", 'f'}
	,{"map", PString, "map2.json", 'm'}
	,{"keymap", PString, "keymap.json", 'k'}
	,{"config", PString, "tankfighter.cfg", 'c'}
};

bool string2long(const char *s, long *v) {
	int cnt=0;
	long vtemp;
	if (!v) v = &vtemp;
	if (sscanf(s, "%ld%n", v, &cnt)<=0 || cnt != int(strlen(s))) {*v=0;return false;}
	return true;
}
bool string2dbl(const char *s, double *v) {
	int cnt=0;
	double vtemp;
	if (!v) v = &vtemp;
	if (sscanf(s, "%lf%n", v, &cnt)<=0 || cnt != int(strlen(s))) {*v=0;return false;}
	return true;
}

long Parameters::getAsLong (const char *name) {
	long res;
	if (!string2long(values[name].c_str(), &res)) return 0;
	return res;
}
double Parameters::getAsDouble (const char *name) {
	double res;
	if (!string2dbl(values[name].c_str(), &res)) return 0;
	return res;
}
bool Parameters::getAsBoolean (const char *name) {
	long res;
	if (!string2long(values[name].c_str(), &res)) return 0;
	return res;
}
std::string Parameters::getAsString (const char *name) {
	return values[name];
}
bool Parameters::set(const char *name, const std::string &value) {
	const char *valstr = value.c_str();
	VarmapIterator it = variables.find(name);
	if (it == variables.end()) {
		fprintf(stderr, "Variable %s not found\n", name);
		return false;
	}
	PVariable &v = (*it).second;
	if (v.datatype == PInteger || v.datatype == PBoolean) {
		long lv=0;
		if (!string2long(valstr, &lv)) {
			fprintf(stderr, "Value %s invalid for variable %s\n", valstr, name);
			return false;
		}
		if (v.datatype == PBoolean && lv != 1 && lv != 0) return false;
	} else if (v.datatype == PDouble) {
		double dv;
		if (!string2dbl(valstr, &dv)) {
			fprintf(stderr, "Value %s invalid for variable %s\n", valstr, name);
			return false;
		}
	} else if (v.datatype != PString) {
		fprintf(stderr, "Internal error: Variable %s has unknown data type\n", name);
		return false;
	}
	values[name]=value;
	return true;
}
Parameters::Parameters() {
	for(size_t i=0; i < sizeof(pdefs)/sizeof(pdefs[0]); i++) {
		ParameterDef &pdef = pdefs[i];
		PVariable v;
		v.datatype = pdef.datatype;
		v.defval = pdef.defval;
		variables.insert(VarmapVal(pdef.name, v));
		set(pdef.name, pdef.defval);
		/*values.insert(pdef.name, pdef.defval);*/
	}
	parseFile(config().c_str());
}
void Parameters::parseCmdline (int argc, char **argv) {
}
void Parameters::parseFile (const char *config_file) {
}

unsigned Parameters::C2S_Packet_interval_US(void) {return 10000;}
double Parameters::resendPacketAfterMS(void) {return 500;}
double Parameters::clientsCleanupIntervalMS(void) {return 1000;}
double Parameters::maxDupPacketTimeSecs(void) {return 10;}
double Parameters::minWallDistance(void) {return 1e-3;}
std::string Parameters::keymap_magic(void) {return "ktank-ctrl-map";}
std::string Parameters::map_magic(void) {return "ktank-map";}
double Parameters::joyTankSpeedCalibration() {return 0.3;}
double Parameters::joyCanonDirectionCalibration() {return 0.15;}
double Parameters::joyDefaultCalibration() {return 0.2;}
int Parameters::serverDiscoveryTimeoutMS() {return 2000;}
int Parameters::serverDiscoveryPeriodMS() {return 500;}
/* constants for networking */
unsigned Parameters::UDP_MTU(void) {return getAsLong("udpMTU");}
unsigned short Parameters::serverPort(void) {return getAsLong("serverPort");}
unsigned short Parameters::clientPort(void) {return getAsLong("clientPort");}
double Parameters::connectionTimeoutInSecs(void) {return getAsDouble("connectionTimeout")*1e-3;}
double Parameters::disconnectionTimeoutMS(void) {return getAsDouble("disconnectionTimeout");}
int Parameters::udpPortRange(void) {return getAsLong("udpPortRange");}

/* constants for game rules */
double Parameters::missileDelayMS(void) {return getAsDouble("missileDelay");}
double Parameters::tankDiameter(void)  {return getAsDouble("tankDiameter");}
std::string Parameters::tankSpriteName(void) {return getAsString("tankSpriteName");}
std::string Parameters::canonSpriteName(void) {return getAsString("canonSpriteName");}
std::string Parameters::missileSpriteName(void) {return getAsString("missileSpriteName");}
double Parameters::canon_rotation_speed(void) {return 1e-3*getAsDouble("canonRotationSpeed");}
double Parameters::tank_rotation_speed(void) {return 1e-3*getAsDouble("tankRotationSpeed");}
double Parameters::linear_tank_speed(void) {return 1e-3*getAsDouble("linearTankSpeed");}
double Parameters::maxMissileLifeDurationMS(void) {return getAsDouble("missileDuration");}
double Parameters::missileSpeed(void) {return 1e-3*getAsDouble("missileSpeed");}
double Parameters::missileDiameter(void) {return getAsDouble("missileDiameter");}

/* constants for game engine */
unsigned Parameters::minFPS(void) {return getAsLong("minFPS");}
unsigned Parameters::maxFPS(void) {return getAsLong("maxFPS");}

/* paths for game data */
std::string Parameters::spritesDirectory(void) {return getAsString("spritesDirectory");}
std::string Parameters::spritesExtension(void) {return getAsString("spritesExtension");}
std::string Parameters::defaultFontName(void) {return getAsString("defaultFontName");}
bool Parameters::fullscreen() {return getAsBoolean("fullscreen");}
std::string Parameters::map() {return getAsString("map");}
std::string Parameters::keymap() {return getAsString("keymap");}
std::string Parameters::config() {return getAsString("config");}
