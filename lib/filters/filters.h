#include <math.h>

typedef struct EffectParams EffectParams;
typedef void (FilterFunc)(XImage *img, EffectParams *p, struct lock *lock);

struct EffectParams {
	FilterFunc *func;
	double parameters[8];
	int num_string_parameters;
	char **string_parameters;
};

static void filter_bloom(XImage *img, EffectParams *p, struct lock *lock);
static void filter_block_shuffle(XImage *img, EffectParams *p, struct lock *lock);
static void filter_blur(XImage *img, EffectParams *p, struct lock *lock);
static void filter_brightness(XImage *img, EffectParams *p, struct lock *lock);
static void filter_channels(XImage *img, EffectParams *p, struct lock *lock);
static void filter_chroma_crawl(XImage *img, EffectParams *p, struct lock *lock);
static void filter_chroma_drift(XImage *img, EffectParams *p, struct lock *lock);
static void filter_color_bleed(XImage *img, EffectParams *p, struct lock *lock);
static void filter_contrast(XImage *img, EffectParams *p, struct lock *lock);
static void filter_crt_effect(XImage *img, EffectParams *p, struct lock *lock);
static void filter_emboss(XImage *img, EffectParams *p, struct lock *lock);
static void filter_dual_kawase_blur(XImage *img, EffectParams *p, struct lock *lock);
static void filter_film_grain(XImage *img, EffectParams *p, struct lock *lock);
static void filter_flip(XImage *img, EffectParams *p, struct lock *lock);
static void filter_ghosting(XImage *img, EffectParams *p, struct lock *lock);
static void filter_gradient_map(XImage *img, EffectParams *p, struct lock *lock);
static void filter_grayscale(XImage *img, EffectParams *p, struct lock *lock);
static void filter_halftone(XImage *img, EffectParams *p, struct lock *lock);
static void filter_horizontal_tear(XImage *img, EffectParams *p, struct lock *lock);
static void filter_hue(XImage *img, EffectParams *p, struct lock *lock);
static void filter_image(XImage *img, EffectParams *p, struct lock *lock);
static void filter_invert(XImage *img, EffectParams *p, struct lock *lock);
static void filter_logo(XImage *img, EffectParams *p, struct lock *lock);
static void filter_noise(XImage *img, EffectParams *p, struct lock *lock);
static void filter_soft_noise(XImage *img, EffectParams *p, struct lock *lock);
static void filter_modulation_glitch(XImage *img, EffectParams *p, struct lock *lock);
static void filter_mosaic(XImage *img, EffectParams *p, struct lock *lock);
static void filter_pixelate(XImage *img, EffectParams *p, struct lock *lock);
static void filter_posterize(XImage *img, EffectParams *p, struct lock *lock);
static void filter_posterize_bit_depth(XImage *img, EffectParams *p, struct lock *lock);
static void filter_random_blocks(XImage *img, EffectParams *p, struct lock *lock);
static void filter_saturation(XImage *img, EffectParams *p, struct lock *lock);
static void filter_scanlines(XImage *img, EffectParams *p, struct lock *lock);
static void filter_screenshot(XImage *img, EffectParams *p, struct lock *lock);
static void filter_sobel(XImage *img, EffectParams *p, struct lock *lock);
static void filter_solid_color(XImage *img, EffectParams *p, struct lock *lock);
static void filter_temperature(XImage *img, EffectParams *p, struct lock *lock);
static void filter_tint(XImage *img, EffectParams *p, struct lock *lock);
static void filter_rain_shift_vert(XImage *img, EffectParams *p, struct lock *lock);
static void filter_rain_shift_horz(XImage *img, EffectParams *p, struct lock *lock);
static void filter_vignette(XImage *img, EffectParams *p, struct lock *lock);
static void filter_vhs_jitter(XImage *img, EffectParams *p, struct lock *lock);
static void filter_vhs_warp_chroma(XImage *img, EffectParams *p, struct lock *lock);
static void filter_wallpaper(XImage *img, EffectParams *p, struct lock *lock);
static void filter_wave_distortion(XImage *img, EffectParams *p, struct lock *lock);

static inline uint8_t clamp255(int v) {
	return v < 0 ? 0 : (v > 255 ? 255 : v);
}

struct effect_map {
	const char *name;
	FilterFunc *func;
};

static const struct effect_map effect_names[] = {
	{ "bloom", filter_bloom },
	{ "block_shuffle", filter_block_shuffle },
	{ "blur", filter_blur },
	{ "brightness", filter_brightness },
	{ "channels", filter_channels },
	{ "chroma_crawl", filter_chroma_crawl },
	{ "chroma_drift", filter_chroma_drift },
	{ "color_bleed", filter_color_bleed },
	{ "contrast", filter_contrast },
	{ "crt_effect", filter_crt_effect },
	{ "dual_kawase_blur", filter_blur },
	{ "emboss", filter_emboss },
	{ "relief", filter_emboss },
	{ "film_grain", filter_film_grain },
	{ "flip", filter_flip },
	{ "ghosting", filter_ghosting },
	{ "gradient_map", filter_gradient_map },
	{ "grayscale", filter_grayscale },
	{ "greyscale", filter_grayscale },
	{ "halftone", filter_halftone },
	{ "horizontal_tear", filter_horizontal_tear },
	{ "hue", filter_hue },
	{ "image", filter_image },
	{ "invert", filter_invert },
	{ "logo", filter_logo },
	{ "modulation_glitch", filter_modulation_glitch },
	{ "mosaic", filter_mosaic },
	{ "noise", filter_noise },
	{ "soft_noise", filter_soft_noise },
	{ "pixelate", filter_pixelate },
	{ "posterize", filter_posterize },
	{ "posterize_bit_depth", filter_posterize_bit_depth },
	{ "random_blocks", filter_random_blocks },
	{ "rain_shift_vert", filter_rain_shift_vert },
	{ "rain_shift_horz", filter_rain_shift_horz },
	{ "saturation", filter_saturation },
	{ "scanlines", filter_scanlines },
	{ "screenshot", filter_screenshot },
	{ "sobel", filter_sobel },
	{ "solid", filter_solid_color },
	{ "solid_color", filter_solid_color },
	{ "temperature", filter_temperature },
	{ "tint", filter_tint },
	{ "vignette", filter_vignette },
	{ "vhs_jitter", filter_vhs_jitter },
	{ "vhs_warp_chroma", filter_vhs_warp_chroma },
	{ "wallpaper", filter_wallpaper },
	{ "wave_distortion", filter_wave_distortion },
	{ NULL, 0 }
};
