#include <libconfig.h>
#include <libgen.h>

#define DIR_MAX 4080
#define PATH_MAX 4096
const char *progname = "dack";
static char *cfg_filename = "dack.cfg";

static NamedColor *colors = NULL;
static EffectParams *background_filters = NULL;
uint64_t settings = 0;
static int num_colors = 0;
static int num_rectangles = 0;
static int num_background_filters = 0;
static int logosize = 75;
static int logow = 12;
static int logoh = 6;
static XRectangle *rectangles = NULL;

#include "lib/libconfig_helper_functions.c"

static void set_config_path(const char* filename, char *config_path, char *config_file);

static void cleanup_config(void);
static void load_config(void);
static void load_fallback_config(void);
static void load_misc(config_t *cfg);
static void load_background(config_t *cfg);
static void load_colors(config_t *cfg);
static void load_functionality(config_t *cfg);
static void load_logo(config_t *cfg);
static void load_filters(config_setting_t *filters_t, int *num_filters, EffectParams **filter_params);

static FilterFunc *parse_effect_filter(const char *name);

int startswith(const char *needle, const char *haystack)
{
	return !strncmp(haystack, needle, strlen(needle));
}

unsigned long
strtopixel(Display *dpy, int cmap, const char *string)
{
	XColor color, dummy;
	XAllocNamedColor(dpy, cmap, string, &color, &dummy);
	return color.pixel;
}

void
set_config_path(const char* filename, char *config_path, char *config_file)
{
    const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");

    if (startswith("/", filename)) {
        char *dname = strdup(filename);
        snprintf(config_path, PATH_MAX, "%s", dirname(dname));
        snprintf(config_file, PATH_MAX, "%s", filename);
        free(dname);
        return;
    }

    if (xdg_config_home && xdg_config_home[0] != '\0') {
        snprintf(config_path, PATH_MAX, "%s/%s", xdg_config_home, progname);
        snprintf(config_file, PATH_MAX, "%s/%s", config_path, filename);
    } else if (home) {
        snprintf(config_path, PATH_MAX, "%s/.config/%s", home, progname);
        snprintf(config_file, PATH_MAX, "%s/%s", config_path, filename);
    }
}

void
load_config(void)
{
	config_t cfg;
	char config_path[DIR_MAX] = {0};
	char config_file[PATH_MAX] = {0};

	const char *envcfg = getenv("DACK_CONFIG_PATH");
	const char *filename = (envcfg && strlen(envcfg) ? envcfg : cfg_filename);

	set_config_path(filename, config_path, config_file);
	config_init(&cfg);
	config_set_include_dir(&cfg, config_path);

	if (config_read_file(&cfg, config_file)) {
		load_functionality(&cfg);
		load_misc(&cfg);
		load_colors(&cfg);
		load_background(&cfg);
		load_logo(&cfg);
	} else if (strcmp(config_error_text(&cfg), "file I/O error")) {
		fprintf(stderr, "Error reading config at %s\n", config_file);
		fprintf(stderr, "%s:%d - %s\n",
			config_error_file(&cfg),
			config_error_line(&cfg),
			config_error_text(&cfg)
		);
	}

	load_fallback_config();
	config_destroy(&cfg);
}

void
load_fallback_config(void)
{

}

void
cleanup_config(void)
{
	int i, s;

	for (i = 0; i < num_colors; i++) {
		free(colors[i].name);
		free(colors[i].value);
		if (colors[i].resource) {
			free(colors[i].resource->name);
			free(colors[i].resource);
		}
	}
	free(colors);

	for (i = 0; i < num_background_filters; i++) {
		for (s = 0; s < background_filters[i].num_string_parameters; s++) {
			free(background_filters[i].string_parameters[s]);
		}
	}
	free(background_filters);
}

void
load_misc(config_t *cfg)
{

}

void
load_colors(config_t *cfg)
{
	int i;
	config_setting_t *cols, *col;
	const char *string;

	cols = config_lookup(cfg, "colors");
	if (!cols || !config_setting_is_list(cols))
		return;
	num_colors = config_setting_length(cols);

	colors = calloc(num_colors, sizeof(NamedColor));

	for (i = 0; i < num_colors; i++) {
		col = config_setting_get_elem(cols, i);
		config_setting_lookup_strdup(col, "name", &colors[i].name);
		config_setting_lookup_strdup(col, "default", &colors[i].value);
		if (config_setting_lookup_string(col, "resource", &string)) {
			colors[i].resource = ecalloc(1, sizeof(ResourcePref));
			colors[i].resource->name = strdup(string);
			colors[i].resource->type = STRING;
			colors[i].resource->dst = &colors[i].value;
		}
	}
}

void
load_functionality(config_t *cfg)
{
	int i, enabled;

	config_setting_t *func_t = config_lookup(cfg, "functionality");
	if (!func_t)
		return;

	for (i = 0; functionality_names[i].name != NULL; i++) {
		if (config_setting_lookup_sloppy_bool(func_t, functionality_names[i].name, &enabled)) {
			setenabled(functionality_names[i].value, enabled);
		}
	}
}

void
load_logo(config_t *cfg)
{
	int i;
	config_setting_t *rectangles_t, *rect_t;

	config_lookup_int(cfg, "logo.size", &logosize);

	rectangles_t = config_lookup(cfg, "logo.rectangles");
	if (!rectangles_t || !config_setting_is_list(rectangles_t))
		return;

	num_rectangles = config_setting_length(rectangles_t);
	rectangles = calloc(num_rectangles, sizeof(XRectangle));

	for (i = 0; i < num_rectangles; i++) {
		rect_t = config_setting_get_elem(rectangles_t, i);
		if (!rect_t || config_setting_length(rect_t) != 4) {
			fprintf(stderr, "logo: invalid rectangle at index %d\n", i);
			continue;
		}

		rectangles[i].x = (short) config_setting_get_int_elem(rect_t, 0);
		rectangles[i].y = (short) config_setting_get_int_elem(rect_t, 1);
		rectangles[i].width = (unsigned short) config_setting_get_int_elem(rect_t, 2);
		rectangles[i].height = (unsigned short) config_setting_get_int_elem(rect_t, 3);

		logow = MAX(logow, rectangles[i].x + rectangles[i].width);
		logoh = MAX(logoh, rectangles[i].y + rectangles[i].height);
	}
}

void
load_background(config_t *cfg)
{
	load_filters(config_lookup(cfg, "background.filters"), &num_background_filters, &background_filters);
}

void
load_filters(config_setting_t *filters_t, int *num_filters, EffectParams **filters)
{
	int i, p, num_params;
	double val;

	const char *string;
	const config_setting_t *filter_t, *params_t, *param_t;

	if (!filters_t || !config_setting_is_list(filters_t))
		return;

	*num_filters = config_setting_length(filters_t);
	*filters = calloc(*num_filters, sizeof(EffectParams));

	for (i = 0; i < *num_filters; i++) {
		filter_t = config_setting_get_elem(filters_t, i);
		if (config_setting_lookup_string(filter_t, "effect", &string)) {
			(*filters)[i].func = parse_effect_filter(string);
		}

		params_t = config_setting_lookup(filter_t, "params");
		if (!params_t)
			continue;

		num_params = setting_length(params_t);
		int str_count = 0;

		/* First pass: count strings */
		for (p = 0; p < num_params; p++) {
			param_t = config_setting_get_elem(params_t, p);
			if (config_setting_type(param_t) == CONFIG_TYPE_STRING &&
					!config_setting_parse_float_string(param_t, &val))
			{
				str_count++;
			}
		}

		if (str_count > 0) {
			(*filters)[i].string_parameters = calloc(str_count, sizeof(char *));
			(*filters)[i].num_string_parameters = str_count;
		}

		/* Second pass: assign values */
		int str_index = 0;
		int p_idx = 0;


		for (p = 0; p < num_params && p_idx < 8; p++) {
			param_t = config_setting_get_elem(params_t, p);

			if (config_setting_type(param_t) == CONFIG_TYPE_STRING &&
					!config_setting_parse_float_string(param_t, &val)) {
				string = config_setting_get_string(param_t);
				(*filters)[i].string_parameters[str_index++] = strdup(string);
			} else {
				(*filters)[i].parameters[p_idx++] = setting_get_float_elem(params_t, p);
			}
		}
	}
}

FilterFunc *
parse_effect_filter(const char *name)
{
	int i;

	if (!name)
		return NULL;

	if (startswith("filter_", name))
		name += 7;

    for (i = 0; effect_names[i].name != NULL; i++) {
        if (!strcmp(effect_names[i].name, name))
            return effect_names[i].func;
    }

    return NULL;
}
