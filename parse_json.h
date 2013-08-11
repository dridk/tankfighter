#ifndef __JSON_PARSE_H__
#define __JSON_PARSE_H__
#include "json.h"

json_value *access_json_hash(const json_value *p, const json_char *key);
bool assertinteger(const char *varname, const json_value *val);
bool try_assign_integer_variable(unsigned short *out, const char *varname, const char *key, const json_value *val);
char *json_string_to_cstring(const json_value *val);

json_value *json_parse_file(const char *path);
bool json_check_magic(const json_value *p, const char *type_name);
#endif
