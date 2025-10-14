
#include "libconfig_helper_functions.h"

int
setting_length(const config_setting_t *cfg)
{
	if (!cfg)
		return 0;

	switch (config_setting_type(cfg)) {
	case CONFIG_TYPE_GROUP:
	case CONFIG_TYPE_LIST:
	case CONFIG_TYPE_ARRAY:
		return config_setting_length(cfg);
	}

	return 1;
}

const char *
setting_get_string_elem(const config_setting_t *cfg, int i)
{
	if (!cfg)
		return NULL;

	switch (config_setting_type(cfg)) {
	case CONFIG_TYPE_GROUP:
	case CONFIG_TYPE_LIST:
	case CONFIG_TYPE_ARRAY:
		return config_setting_get_string_elem(cfg, i);
	}

	return config_setting_get_string(cfg);
}

int
setting_get_int_elem(const config_setting_t *cfg, int i)
{
	if (!cfg)
		return 0;

	switch (config_setting_type(cfg)) {
	case CONFIG_TYPE_GROUP:
	case CONFIG_TYPE_LIST:
	case CONFIG_TYPE_ARRAY:
		return config_setting_get_int_elem(cfg, i);
	}

	return config_setting_get_int(cfg);
}

double
setting_get_float_elem(const config_setting_t *cfg, int i)
{
	if (!cfg)
		return 0;

	double ret = 0.0;

	switch (config_setting_type(cfg)) {
	case CONFIG_TYPE_GROUP:
	case CONFIG_TYPE_LIST:
	case CONFIG_TYPE_ARRAY:
		return setting_get_float_elem(config_setting_get_elem(cfg, i), 0);
	case CONFIG_TYPE_FLOAT:
		return config_setting_get_float(cfg);
	case CONFIG_TYPE_INT:
		return (double)config_setting_get_int(cfg);
	case CONFIG_TYPE_STRING:
		config_setting_parse_float_string(cfg, &ret);
		return ret;
	}

	return config_setting_get_float(cfg);
}

const config_setting_t *
setting_get_elem(const config_setting_t *cfg, int i)
{
	if (!cfg)
		return NULL;

	switch (config_setting_type(cfg)) {
	case CONFIG_TYPE_GROUP:
	case CONFIG_TYPE_LIST:
	case CONFIG_TYPE_ARRAY:
		return config_setting_get_elem(cfg, i);
	}

	return cfg;
}

int
config_setting_parse_float_string(const config_setting_t *cfg, double *value)
{
	int i;
	const char *string = config_setting_get_string(cfg);

	if (!string)
		return 0;

	for (i = 0; i < num_colors; i++) {
		if (strcmp(colors[i].name, string) == 0) {
			*value = (double)i;
			return 1;
		}
	}

	i = parse_blend_name(string);
	if (i > -1) {
		*value = (double)i;
		return 1;
	}

	return 0;
}

int
config_lookup_strdup(const config_t *cfg, const char *name, char **strptr)
{
	return _config_setting_strdup_string(config_lookup(cfg, name), strptr);
}


int
config_setting_lookup_strdup(const config_setting_t *cfg, const char *name, char **strptr)
{
	return _config_setting_strdup_string(config_setting_lookup(cfg, name), strptr);
}

int
_config_setting_strdup_string(const config_setting_t *cfg_item, char **strptr)
{
	if (!cfg_item)
		return 0;

	const char *string = config_setting_get_string(cfg_item);

	if (!string)
		return 0;

	free(*strptr);
	*strptr = strdup(string);
	return 1;
}

int
config_lookup_sloppy_bool(const config_t *cfg, const char *name, int *ptr)
{
	return _config_setting_get_sloppy_bool(config_lookup(cfg, name), ptr);
}

int
config_setting_lookup_sloppy_bool(const config_setting_t *cfg, const char *name, int *ptr)
{
	return _config_setting_get_sloppy_bool(config_setting_lookup(cfg, name), ptr);
}

int
_config_setting_get_sloppy_bool(const config_setting_t *cfg_item, int *ptr)
{
	const char *string;

	if (!cfg_item)
		return 0;

	switch (config_setting_type(cfg_item)) {
	case CONFIG_TYPE_INT:
		*ptr = config_setting_get_int(cfg_item);
		return 1;
	case CONFIG_TYPE_STRING:
		string = config_setting_get_string(cfg_item);

		if (string && strlen(string)) {
			char a = tolower(string[0]);
			/* Match for positives like "true", "yes" and "on" */
			*ptr = (a == 't' || a == 'y' || !strcasecmp(string, "on"));
			return 1;
		}
		break;
	case CONFIG_TYPE_BOOL:
		*ptr = config_setting_get_bool(cfg_item);
		return 1;
	}

	return 0;
}

int
config_lookup_simple_float(const config_t *cfg, const char *name, float *floatptr)
{
	return _config_setting_get_simple_float(config_lookup(cfg, name), floatptr);
}

int
config_setting_lookup_simple_float(const config_setting_t *cfg, const char *name, float *floatptr)
{
	return _config_setting_get_simple_float(config_setting_lookup(cfg, name), floatptr);
}

int
_config_setting_get_simple_float(const config_setting_t *cfg_item, float *floatptr)
{
	if (!cfg_item)
		return 0;

	double value = config_setting_get_float(cfg_item);

	*floatptr = (float)value;
	return 1;
}

int
config_lookup_unsigned_int(const config_t *cfg, const char *name, unsigned int *ptr)
{
	return _config_setting_get_unsigned_int(config_lookup(cfg, name), ptr);
}

int
config_setting_lookup_unsigned_int(const config_setting_t *cfg, const char *name, unsigned int *ptr)
{
	return _config_setting_get_unsigned_int(config_setting_lookup(cfg, name), ptr);
}

int
_config_setting_get_unsigned_int(const config_setting_t *cfg_item, unsigned int *ptr)
{
	if (!cfg_item)
		return 0;

	int integer = config_setting_get_int(cfg_item);

	if (integer >= 0) {
		*ptr = (unsigned int)integer;
		return 1;
	}

	return 1;
}
