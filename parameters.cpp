#include "parameters.h"
#include <vector>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "misc.h"
#include <iostream>
#include "parse_json.h"

Parameters parameters;

static struct ParameterDef {
	const char 	*name;
	PType		datatype;
	const char     *defval;
	char		cmdline;
} pdefs[]={
/* network */
	 {"udpMTU", PInteger, "512"}
	,{"networkFPS", PDouble, "60"}
	,{"serverPort", PInteger, "1330"}
	,{"clientPort", PInteger, "1320"}
	,{"connectionTimeout", PDouble, "10000"}
	,{"disconnectionTimeout", PDouble, "2000"}
	,{"broadcastPeriod", PDouble, "500"}
	,{"udpPortRange", PInteger, "100"}
	,{"server", PBoolean, "0"}
	,{"join", PString, ""}

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

/* engine */
	,{"minFPS", PDouble, "15"}
	,{"maxFPS", PDouble, "60"}
	,{"width", PInteger, "0"} /* 0 = desktop size */
	,{"height", PInteger, "0"}

/* file system */
	,{"spritesDirectory", PString, "sprites/"}
	,{"spritesExtension", PString, ".png"}
	,{"defaultFontName", PString, "sans-serif:slant=roman:weight=normal:lang=en_US:scalable=true"}

	,{"fullscreen", PBoolean, "1", 'f'}
	,{"map", PString, "map2.json", 'm'}
	,{"keymap", PString, "keymap.json", 'k'}
	,{"config", PString, "tankfighter.cfg", 'c'}
	,{"help", PBoolean, "0", 'h'}
	,{"noGUI", PBoolean, "0"}
};

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
bool Parameters::parseCmdline (int argc, char **argv) {
	int index = 0;
	std::vector<struct option> options(variables.size());
	size_t i=0;
	for(VarmapIterator it=variables.begin(); it != variables.end(); ++it) {
		VarmapVal v = *it;
		struct option o;
		o.name = v.first.c_str();
		o.has_arg = (v.second.datatype == PBoolean ? optional_argument  : required_argument);
		o.flag = NULL;
		o.val = 0;
		options[i++] = o;
	}
	while (getopt_long(argc, argv, "", &options[0], &index) != -1) {
		if (optarg) {
			set(options[index].name, optarg);
		} else {
			set(options[index].name, "1");
		}
	}
	if (getAsBoolean("help")) {
		outputHelpString(std::cout);
		return false;
	}
	return true;
}
void Parameters::outputHelpString (std::ostream &out) {
	for(VarmapIterator it=variables.begin(); it != variables.end(); ++it) {
		VarmapVal v = *it;
		PType dt = v.second.datatype;
		std::string type;
		if (dt == PString) type = "string";
		if (dt == PBoolean) type = "boolean";
		if (dt == PInteger) type = "integer";
		if (dt == PDouble) type = "float";
		out << "\t--" << v.first << " " << type << " with default = " << v.second.defval << "\n";
	}
}
bool Parameters::parseFile (const char *config_file) {
	json_value *p = json_parse_file(config_file, parameters.config_magic().c_str());
	if (!p) return false;
	const json_value *map = access_json_hash(p, "parameters");
	if ((!map) || map->type != json_object) {
		json_value_free(p);
		fprintf(stderr, "parameters hash must be present and must be an associative array\n");
		return false;
	}
	for (unsigned i=0; i < map->u.object.length; i++) {
		const char *variable=map->u.object.values[i].name;
		const json_value *val = map->u.object.values[i].value;
		set(variable, val);
	}

	json_value_free(p);
	return true;
}
bool Parameters::set(const char *name, const json_value *value) {
	VarmapIterator it = variables.find(name);
	std::string sval;
	if (it == variables.end()) {
		fprintf(stderr, "Variable %s doesn't exist\n", name);
		return false;
	}
	PVariable &v = (*it).second;
	int d1 = v.datatype, d2 = value->type;
	if (!(d1 == d2 || (d1 == PBoolean && d2 == json_integer) || d2 == json_string)) {
		fprintf(stderr, "Variable assignment type mismatch for %s\n", name);
		return false;
	}
	if (value->type == json_boolean) sval = tostring((int)value->u.boolean);
	if (value->type == json_integer) sval = tostring(value->u.integer);
	if (value->type == json_double)  sval = tostring(value->u.dbl);
	if (value->type == json_string)  {
		char *sp = json_string_to_cstring(value);
		if (sp) sval=sp;
		free(sp);
	}
	return set(name, sval);
}

sf::Int64 Parameters::C2S_Packet_interval_US(void) {
	double fps = getAsDouble("networkFPS");
	if (fps <= 0.1) fps = 0.1;
	return (1/fps)*1e6;
}
double Parameters::resendPacketAfterMS(void) {return 500;}
double Parameters::clientsCleanupIntervalMS(void) {return 1000;}
double Parameters::maxDupPacketTimeSecs(void) {return 10;}
double Parameters::minWallDistance(void) {return 1e-3;}
std::string Parameters::keymap_magic(void) {return "ktank-ctrl-map";}
std::string Parameters::map_magic(void) {return "ktank-map";}
std::string Parameters::config_magic(void) {return "ktank-config";}
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
bool Parameters::startServer() {return getAsBoolean("server");}
std::string Parameters::joinAddress() {return getAsString("join");}
long Parameters::screenWidth() {return getAsLong("width");}
long Parameters::screenHeight() {return getAsLong("height");}
sf::VideoMode Parameters::getVideoMode() {
	sf::VideoMode mode;
	long w = screenWidth(), h = screenHeight();
	if (w == 0 && h == 0) return sf::VideoMode::getDesktopMode();
	if (h == 0) {
		if (w >= 1920) h = w*9/16;
		else h = w*3/4;
	}
	return sf::VideoMode(w, h);
}
bool Parameters::noGUI() {return getAsBoolean("noGUI");}
