#include "parse_json.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "misc.h"

json_value *access_json_hash(const json_value *p, const json_char *key) {
	if (p->type != json_object) return NULL;
	for (unsigned i=0; i < p->u.object.length; i++) {
		if (strcmp(p->u.object.values[i].name, key)==0) {
			return p->u.object.values[i].value;
		}
	}
	return NULL;
}

bool assertinteger(const char *varname, const json_value *val) {
	if (val->type != json_integer) {
		fprintf(stderr, "Expected integer for parameter %s\n", varname);
		return false;
	}
	return true;
}
bool try_assign_integer_variable(unsigned short *out, const char *varname, const char *key, const json_value *val) {
	if (strcmp(key, varname)==0) {
		if (assertinteger(varname, val)) {*out = val->u.integer;return true;}
		else return false;
	}
	return true;
}
bool try_assign_double_variable(double *out, const char *varname, const char *key, const json_value *val) {
	if (strcmp(key, varname)==0) {
		if (val->type == json_double) {*out = val->u.dbl;return true;}
		else if (val->type == json_integer) {*out = val->u.integer;return true;}
		else {
			fprintf(stderr, "Expected numeric parameter %s\n", varname);
			return false;
		}
	}
	return true;
}

char *json_string_to_cstring(const json_value *val) {
	if (val->type != json_string) return NULL;
	const char *p = val->u.string.ptr;
	size_t     ln = val->u.string.length;
	char *res = (char*)malloc(ln+1);
	if (!res) return NULL;
	memcpy(res, p, ln);
	res[ln]=0;
	return res;
}
json_value *json_parse_file(const char *path, const char *type_name) {
	unsigned long file_size=0;
	char *json = (char*)load_file(path, &file_size);
	if (!json) {
		fprintf(stderr, "Failed to load json file %s\n", path);
		return NULL;
	}
	json_value *p = json_parse(json, file_size);
	free(json);
	if (!p) {
		fprintf(stderr, "Failed to parse json file %s\n", path);
		return NULL;
	}
	if (type_name && !json_check_magic(p, type_name)) {
		json_value_free(p);
		return NULL;
	}
	return p;
}
bool json_check_magic(const json_value *p, const char *type_name) {
	if (p->type != json_object) {
		fprintf(stderr, "JSON must be an associative array!");
		return false;
	}
	
	const json_value *map_type = access_json_hash(p, "type");
	if (!map_type) {
		fprintf(stderr, "JSON is not a %s (no type field)!", type_name);
		return false;
	}
	if (!(map_type->type == json_string && strcmp(map_type->u.string.ptr, type_name)==0)) {
		fprintf(stderr, "JSON is not a %s (type field is not %s)!", type_name, type_name);
		return false;
	}
	return true;
}
