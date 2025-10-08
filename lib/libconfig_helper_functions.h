/* libconfig helper functions */
static int setting_length(const config_setting_t *cfg);
const char *setting_get_string_elem(const config_setting_t *cfg, int i);
static int setting_get_int_elem(const config_setting_t *cfg, int i);
static double setting_get_float_elem(const config_setting_t *cfg, int i);
const config_setting_t *setting_get_elem(const config_setting_t *cfg, int i);
static double config_setting_parse_float_string(const config_setting_t *cfg);

static int config_lookup_strdup(const config_t *cfg, const char *name, char **strptr);
static int config_setting_lookup_strdup(const config_setting_t *cfg, const char *name, char **strptr);
static int _config_setting_strdup_string(const config_setting_t *cfg_item, char **strptr);

static int config_lookup_sloppy_bool(const config_t *cfg, const char *name, int *ptr);
static int config_setting_lookup_sloppy_bool(const config_setting_t *cfg, const char *name, int *ptr);
static int _config_setting_get_sloppy_bool(const config_setting_t *cfg, int *ptr);

static int config_lookup_simple_float(const config_t *cfg, const char *name, float *floatptr);
static int config_setting_lookup_simple_float(const config_setting_t *cfg, const char *name, float *floatptr);
static int _config_setting_get_simple_float(const config_setting_t *cfg_item, float *floatptr);

static int config_lookup_unsigned_int(const config_t *cfg, const char *name, unsigned int *ptr);
static int config_setting_lookup_unsigned_int(const config_setting_t *cfg, const char *name, unsigned int *ptr);
static int _config_setting_get_unsigned_int(const config_setting_t *cfg_item, unsigned int *ptr);
