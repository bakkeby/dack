/* Red, Green, Blue */
typedef struct {
	double r, g, b;
} RGB;

/* Hue, Saturation, Luminosity */
typedef struct {
	double h, s, l;
} HSL;

/* Lightness, Chroma, Hue (in degrees) */
typedef struct {
	double L, C, H;
} LCH;

/* Lightness, green-red axis (a), blue-yellow axis (b) */
typedef struct {
	double L, a, b;
} LAB;

typedef enum {
	BLEND_NORMAL,
	BLEND_MULTIPLY,
	BLEND_SCREEN,
	BLEND_OVERLAY,
	BLEND_DARKEN,
	BLEND_LIGHTEN,
	BLEND_DIFFERENCE,
	BLEND_ADD,
	BLEND_SUBTRACT,
	BLEND_REPLACE,
	BLEND_DISSOLVE,
	BLEND_BEHIND,
	BLEND_ERASE,
	BLEND_MERGE,
	BLEND_SPLIT,
	BLEND_ATTRITION,
	BLEND_GRAIN_EXTRACT,
	BLEND_GRAIN_MERGE,
	BLEND_DIVIDE,
	BLEND_COLOR_ERASE,
	BLEND_COLOR_BURN,
	BLEND_COLOR_DODGE,
	BLEND_LINEAR_BURN,
	BLEND_LINEAR_DODGE,
	BLEND_SOFT_LIGHT,
	BLEND_HARD_LIGHT,
	BLEND_EXCLUSION,
	BLEND_PIN_LIGHT,
	BLEND_VIVID_LIGHT,
	BLEND_LINEAR_LIGHT,
	BLEND_HARD_MIX,
	BLEND_MIN,
	BLEND_MAX,
	BLEND_HSV_HUE,
	BLEND_HSV_SATURATION,
	BLEND_HSV_COLOR,
	BLEND_HSV_LUMINOSITY,
	BLEND_LCH_HUE,
	BLEND_LCH_CHROMA,
	BLEND_LCH_COLOR,
	BLEND_LCH_LIGHTNESS,
} BlendMode;

typedef enum {
	CENTER,
	TILE,
	RELATIVE,
} BlendPosition;

typedef struct {
	BlendMode blend_mode;
	float blend_strength;
	BlendPosition blend_position;
	int x;
	int y;
} BlendOptions;

struct blend_map {
	const char *name;
	BlendMode mode;
};

static const struct blend_map blend_names[] = {
	{ "NORMAL", BLEND_NORMAL },
	{ "MULTIPLY", BLEND_MULTIPLY },
	{ "SCREEN", BLEND_SCREEN },
	{ "OVERLAY", BLEND_OVERLAY },
	{ "DARKEN", BLEND_DARKEN },
	{ "LIGHTEN", BLEND_LIGHTEN },
	{ "DIFFERENCE", BLEND_DIFFERENCE },
	{ "ADD", BLEND_ADD },
	{ "SUBTRACT", BLEND_SUBTRACT },
	{ "REPLACE", BLEND_REPLACE },
	{ "DISSOLVE", BLEND_DISSOLVE },
	{ "BEHIND", BLEND_BEHIND },
	{ "ERASE", BLEND_ERASE },
	{ "MERGE", BLEND_MERGE },
	{ "SPLIT", BLEND_SPLIT },
	{ "ATTRITION", BLEND_ATTRITION },
	{ "GRAIN_EXTRACT", BLEND_GRAIN_EXTRACT },
	{ "GRAIN_MERGE", BLEND_GRAIN_MERGE },
	{ "DIVIDE", BLEND_DIVIDE },
	{ "COLOR_ERASE", BLEND_COLOR_ERASE },
	{ "COLOR_BURN", BLEND_COLOR_BURN },
	{ "COLOR_DODGE", BLEND_COLOR_DODGE },
	{ "LINEAR_BURN", BLEND_LINEAR_BURN },
	{ "LINEAR_DODGE", BLEND_LINEAR_DODGE },
	{ "SOFT_LIGHT", BLEND_SOFT_LIGHT },
	{ "HARD_LIGHT", BLEND_HARD_LIGHT },
	{ "EXCLUSION", BLEND_EXCLUSION },
	{ "PIN_LIGHT", BLEND_PIN_LIGHT },
	{ "VIVID_LIGHT", BLEND_VIVID_LIGHT },
	{ "LINEAR_LIGHT", BLEND_LINEAR_LIGHT },
	{ "HARD_MIX", BLEND_HARD_MIX },
	{ "MAX", BLEND_MAX },
	{ "HSV_HUE", BLEND_HSV_HUE },
	{ "HSV_SATURATION", BLEND_HSV_SATURATION },
	{ "HSV_COLOR", BLEND_HSV_COLOR },
	{ "HSV_COLOUR", BLEND_HSV_COLOR },
	{ "HSV_LUMINOSITY", BLEND_HSV_LUMINOSITY },
	{ "LCH_HUE", BLEND_LCH_HUE },
	{ "LCH_CHROMA", BLEND_LCH_CHROMA },
	{ "LCH_COLOR", BLEND_LCH_COLOR },
	{ "LCH_COLOUR", BLEND_LCH_COLOR },
	{ "LCH_LIGHTNESS", BLEND_LCH_LIGHTNESS },
	{ NULL, 0 }
};

static HSL rgb_to_hsl(RGB c);
static RGB hsl_to_rgb(HSL c);
static LCH rgb_to_lch(RGB c);
static RGB lch_to_rgb(LCH c);
static RGB blend_hsl_mode(RGB dst, RGB src, int mode);
static RGB blend_lch_mode(RGB dst, RGB src, int mode);
static inline void blend_pixel(uint8_t *dst, uint8_t sr, uint8_t sg, uint8_t sb, uint8_t sa, float blend, BlendMode mode);
static void blend_images(XImage *dst, XImage *src, Monitor *m, BlendOptions *options);
