/* Reference white (D65) */
#define REF_X  95.047
#define REF_Y 100.000
#define REF_Z 108.883

/* Return random float in [0.0, 1.0) */
static inline float randf(void)
{
	return (float)rand() / (float)(RAND_MAX + 1.0);
}

/* RGB (0–1) -> XYZ */
static void rgb_to_xyz(RGB in, double *X, double *Y, double *Z)
{
	double r = in.r, g = in.g, b = in.b;

	/* sRGB gamma correction */
	r = (r > 0.04045) ? pow((r + 0.055) / 1.055, 2.4) : r / 12.92;
	g = (g > 0.04045) ? pow((g + 0.055) / 1.055, 2.4) : g / 12.92;
	b = (b > 0.04045) ? pow((b + 0.055) / 1.055, 2.4) : b / 12.92;

	r *= 100.0;
	g *= 100.0;
	b *= 100.0;

	/* Observer = 2°, Illuminant = D65 */
	*X = r * 0.4124 + g * 0.3576 + b * 0.1805;
	*Y = r * 0.2126 + g * 0.7152 + b * 0.0722;
	*Z = r * 0.0193 + g * 0.1192 + b * 0.9505;
}

/* XYZ -> LAB */
static LAB xyz_to_lab(double X, double Y, double Z)
{
	X /= REF_X;
	Y /= REF_Y;
	Z /= REF_Z;

	double fx = (X > 0.008856) ? cbrt(X) : (7.787 * X) + (16.0 / 116.0);
	double fy = (Y > 0.008856) ? cbrt(Y) : (7.787 * Y) + (16.0 / 116.0);
	double fz = (Z > 0.008856) ? cbrt(Z) : (7.787 * Z) + (16.0 / 116.0);

	LAB out;
	out.L = (116.0 * fy) - 16.0;
	out.a = 500.0 * (fx - fy);
	out.b = 200.0 * (fy - fz);
	return out;
}

/* LAB -> LCH */
static LCH lab_to_lch(LAB in)
{
	LCH out;
	out.L = in.L;
	out.C = sqrt(in.a * in.a + in.b * in.b);
	out.H = atan2(in.b, in.a) * (180.0 / M_PI);
	if (out.H < 0.0)
		out.H += 360.0;
	return out;
}

/* LCH -> LAB */
static LAB lch_to_lab(LCH in)
{
	LAB out;
	double Hrad = in.H * (M_PI / 180.0);
	out.L = in.L;
	out.a = cos(Hrad) * in.C;
	out.b = sin(Hrad) * in.C;
	return out;
}

/* LAB -> XYZ */
static void lab_to_xyz(LAB in, double *X, double *Y, double *Z)
{
	double fy = (in.L + 16.0) / 116.0;
	double fx = in.a / 500.0 + fy;
	double fz = fy - in.b / 200.0;

	double fx3 = fx * fx * fx;
	double fz3 = fz * fz * fz;
	double fy3 = fy * fy * fy;

	if (fy3 > 0.008856)
		*Y = fy3;
	else
		*Y = (fy - 16.0 / 116.0) / 7.787;

	if (fx3 > 0.008856)
		*X = fx3;
	else
		*X = (fx - 16.0 / 116.0) / 7.787;

	if (fz3 > 0.008856)
		*Z = fz3;
	else
		*Z = (fz - 16.0 / 116.0) / 7.787;

	*X *= REF_X;
	*Y *= REF_Y;
	*Z *= REF_Z;
}

/* XYZ -> RGB */
static RGB xyz_to_rgb(double X, double Y, double Z)
{
	X /= 100.0;
	Y /= 100.0;
	Z /= 100.0;

	RGB out;
	out.r = X *  3.2406 + Y * -1.5372 + Z * -0.4986;
	out.g = X * -0.9689 + Y *  1.8758 + Z *  0.0415;
	out.b = X *  0.0557 + Y * -0.2040 + Z *  1.0570;

	out.r = (out.r > 0.0031308) ? (1.055 * pow(out.r, 1.0 / 2.4) - 0.055) : (12.92 * out.r);
	out.g = (out.g > 0.0031308) ? (1.055 * pow(out.g, 1.0 / 2.4) - 0.055) : (12.92 * out.g);
	out.b = (out.b > 0.0031308) ? (1.055 * pow(out.b, 1.0 / 2.4) - 0.055) : (12.92 * out.b);

	out.r = CLAMP(out.r, 0.0, 1.0);
	out.g = CLAMP(out.g, 0.0, 1.0);
	out.b = CLAMP(out.b, 0.0, 1.0);
	return out;
}

static LCH rgb_to_lch(RGB c)
{
	double X, Y, Z;
	rgb_to_xyz(c, &X, &Y, &Z);
	LAB lab = xyz_to_lab(X, Y, Z);
	return lab_to_lch(lab);
}

static RGB lch_to_rgb(LCH c)
{
	LAB lab = lch_to_lab(c);
	double X, Y, Z;
	lab_to_xyz(lab, &X, &Y, &Z);
	return xyz_to_rgb(X, Y, Z);
}

/* Convert RGB [0,1] → HSL [0,1] */
static HSL rgb_to_hsl(RGB c)
{
	double max = fmax(fmax(c.r, c.g), c.b);
	double min = fmin(fmin(c.r, c.g), c.b);
	double h, s, l = (max + min) / 2.0;

	if (max == min) {
		h = s = 0.0; /* achromatic */
	} else {
		double d = max - min;
		s = l > 0.5 ? d / (2.0 - max - min) : d / (max + min);

		if (max == c.r)
			h = (c.g - c.b) / d + (c.g < c.b ? 6.0 : 0.0);
		else if (max == c.g)
			h = (c.b - c.r) / d + 2.0;
		else
			h = (c.r - c.g) / d + 4.0;

		h /= 6.0;
	}

	return (HSL){ h, s, l };
}

/* Helper for HSL→RGB conversion */
static double hue_to_rgb(double p, double q, double t)
{
	if (t < 0.0) t += 1.0;
	if (t > 1.0) t -= 1.0;
	if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
	if (t < 1.0 / 2.0) return q;
	if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
	return p;
}

/* Convert HSL [0,1] → RGB [0,1] */
static RGB hsl_to_rgb(HSL c)
{
	double r, g, b;

	if (c.s == 0.0) {
		r = g = b = c.l; /* gray */
	} else {
		double q = c.l < 0.5 ? c.l * (1.0 + c.s)
		                     : c.l + c.s - c.l * c.s;
		double p = 2.0 * c.l - q;
		r = hue_to_rgb(p, q, c.h + 1.0 / 3.0);
		g = hue_to_rgb(p, q, c.h);
		b = hue_to_rgb(p, q, c.h - 1.0 / 3.0);
	}

	return (RGB){ r, g, b };
}

/* Blend mode: hue/saturation/color/luminosity */
static RGB blend_hsl_mode(RGB dst, RGB src, int mode)
{
	HSL d = rgb_to_hsl(dst);
	HSL s = rgb_to_hsl(src);
	HSL r = d; /* start from destination */

	switch (mode) {
	case BLEND_HSV_HUE:
		r.h = s.h;
		break;
	case BLEND_HSV_SATURATION:
		r.s = s.s;
		break;
	case BLEND_HSV_COLOR:
		r.h = s.h;
		r.s = s.s;
		break;
	case BLEND_HSV_LUMINOSITY:
		r.l = s.l;
		break;
	default:
		break;
	}

	return hsl_to_rgb(r);
}

/* Blend mode: lightness/chroma/hue */
static RGB blend_lch_mode(RGB dst, RGB src, int mode)
{
	LCH d = rgb_to_lch(dst);
	LCH s = rgb_to_lch(src);
	LCH r = d; /* start from destination */

	switch (mode) {
	case BLEND_LCH_HUE:
		r.H = s.H;
		if (r.H < 0.0) r.H += 360.0;
		else if (r.H >= 360.0) r.H -= 360.0;
		break;
	case BLEND_LCH_CHROMA:
		r.C = s.C;
		break;
	case BLEND_LCH_COLOR:
		r.C = s.C;
		r.H = s.H;
		break;
	default:
	case BLEND_LCH_LIGHTNESS:
		r.L = s.L;
		break;
	}

	return lch_to_rgb(r);
}

static inline void
blend_pixel(uint8_t *dst, uint8_t src_r8, uint8_t src_g8, uint8_t src_b8, uint8_t src_a8, float blend_strength, BlendMode mode)
{
	float alpha = (src_a8 / 255.0f) * blend_strength;

	/* Normalised values (between 0.0 and 1.0) */
	float dr = dst[2] / 255.0f;
	float dg = dst[1] / 255.0f;
	float db = dst[0] / 255.0f;

	float sr = src_r8 / 255.0f;
	float sg = src_g8 / 255.0f;
	float sb = src_b8 / 255.0f;

	float r, g, b;

	switch (mode) {
	case BLEND_MULTIPLY:
		r = dr * sr;
		g = dg * sg;
		b = db * sb;
		break;
	case BLEND_SCREEN:
		r = 1.0f - (1.0f - dr) * (1.0f - sr);
		g = 1.0f - (1.0f - dg) * (1.0f - sg);
		b = 1.0f - (1.0f - db) * (1.0f - sb);
		break;
	case BLEND_HARD_MIX:
		r = (sr + dr < 1.0f ? 0.0f : 1.0f);
		g = (sg + dg < 1.0f ? 0.0f : 1.0f);
		b = (sb + db < 1.0f ? 0.0f : 1.0f);
		break;
	case BLEND_MAX:
		r = MAX(sr, dr);
		g = MAX(sg, dg);
		b = MAX(sb, db);
		break;
	case BLEND_MIN:
		r = MIN(sr, dr);
		g = MIN(sg, dg);
		b = MIN(sb, db);
		break;
	case BLEND_OVERLAY:
		r = (dr < 0.5f) ? (2.0f * dr * sr) : (1.0f - 2.0f * (1.0f - dr) * (1.0f - sr));
		g = (dg < 0.5f) ? (2.0f * dg * sg) : (1.0f - 2.0f * (1.0f - dg) * (1.0f - sg));
		b = (db < 0.5f) ? (2.0f * db * sb) : (1.0f - 2.0f * (1.0f - db) * (1.0f - sb));
		break;
	case BLEND_SOFT_LIGHT:
		r = (sr < 0.5f) ? (dr - (1.0f - 2.0f * sr) * dr * (1.0f - dr)) :
			(dr + (2.0f * sr - 1.0f) * (sqrtf(dr) - dr));
		g = (sg < 0.5f) ? (dg - (1.0f - 2.0f * sg) * dg * (1.0f - dg)) :
			(dg + (2.0f * sg - 1.0f) * (sqrtf(dg) - dg));
		b = (sb < 0.5f) ? (db - (1.0f - 2.0f * sb) * db * (1.0f - db)) :
			(db + (2.0f * sb - 1.0f) * (sqrtf(db) - db));
		break;
	case BLEND_HARD_LIGHT:
		r = (sr < 0.5f) ? (2.0f * dr * sr) : (1.0f - 2.0f * (1.0f - dr) * (1.0f - sr));
		g = (sg < 0.5f) ? (2.0f * dg * sg) : (1.0f - 2.0f * (1.0f - dg) * (1.0f - sg));
		b = (sb < 0.5f) ? (2.0f * db * sb) : (1.0f - 2.0f * (1.0f - db) * (1.0f - sb));
		break;
	case BLEND_PIN_LIGHT:
		r = (sr < 0.5) ? ((dr < sr * 2) ? sr * 2 : dr) : ((dr > (sr - 0.5) * 2) ? (sr - 0.5) * 2 : dr);
		g = (sg < 0.5) ? ((dg < sg * 2) ? sg * 2 : dg) : ((dg > (sg - 0.5) * 2) ? (sg - 0.5) * 2 : dg);
		b = (sb < 0.5) ? ((db < sb * 2) ? sb * 2 : db) : ((db > (sb - 0.5) * 2) ? (sb - 0.5) * 2 : db);
		break;
	case BLEND_VIVID_LIGHT: {
		/* Burn and dodge values */
		int sr_burn = 2 * sr;
		int sr_dodge = 2 * (sr - 0.5);
		int sg_burn = 2 * sg;
		int sg_dodge = 2 * (sg - 0.5);
		int sb_burn = 2 * sb;
		int sb_dodge = 2 * (sb - 0.5);

		r = (sr < 0.5)
		  ? (sr_burn == 0) ? 0 : CLAMP(1.0 - ((1.0 - dr) * 1.0/sr_burn), 0.0f, 1.0f)
		  : (sr_dodge == 1.0) ? 1.0 : CLAMP(dr / (1.0 - sr_dodge), 0.0f, 1.0f);
		g = (sg < 0.5)
		  ? (sg_burn == 0) ? 0 : CLAMP(1.0 - ((1.0 - dg) * 1.0/sg_burn), 0.0f, 1.0f)
		  : (sg_dodge == 1.0) ? 1.0 : CLAMP(dg / (1.0 - sg_dodge), 0.0f, 1.0f);
		b = (sb < 0.5)
		  ? (sb_burn == 0) ? 0 : CLAMP(1.0 - ((1.0 - db) * 1.0/sb_burn), 0.0f, 1.0f)
		  : (sb_dodge == 1.0) ? 1.0 : CLAMP(db / (1.0 - sb_dodge), 0.0f, 1.0f);
		break;
	}
	case BLEND_LINEAR_LIGHT:
		r = CLAMP(dr + 2 * sr - 1.0, 0.0f, 1.0f);
		g = CLAMP(dg + 2 * sg - 1.0, 0.0f, 1.0f);
		b = CLAMP(db + 2 * sb - 1.0, 0.0f, 1.0f);
		break;
	case BLEND_DIFFERENCE:
		r = fabsf(dr - sr);
		g = fabsf(dg - sg);
		b = fabsf(db - sb);
		break;
	case BLEND_EXCLUSION:
		r = (dr + sr - 2 * dr * sr);
		g = (dg + sg - 2 * dg * sg);
		b = (db + sb - 2 * db * sb);
		break;
	case BLEND_ADD:
		r = fminf(dr + sr, 1.0f);
		g = fminf(dg + sg, 1.0f);
		b = fminf(db + sb, 1.0f);
		break;
	case BLEND_SUBTRACT:
		r = fmaxf(dr - sr, 0.0f);
		g = fmaxf(dg - sg, 0.0f);
		b = fmaxf(db - sb, 0.0f);
		break;
	case BLEND_REPLACE:
		/* Directly replace destination with source (ignores alpha) */
		r = sr;
		g = sg;
		b = sb;
		break;
	case BLEND_DISSOLVE:
		/* Randomly replace pixels based on blend factor (alpha) */
		if (randf() < alpha) {
			r = sr;
			g = sg;
			b = sb;
		} else {
			r = dr;
			g = dg;
			b = db;
		}
		break;
	case BLEND_ERASE:
		/* Subtracts alpha, making dest more transparent — here we approximate by darkening */
		r = dr * (1.0f - alpha);
		g = dg * (1.0f - alpha);
		b = db * (1.0f - alpha);
		break;
	case BLEND_BEHIND:
		/* Paints only where dest is transparent — approximation */
		r = dr * (1.0f - alpha) + sr * (1.0f - dr);
		g = dg * (1.0f - alpha) + sg * (1.0f - dg);
		b = db * (1.0f - alpha) + sb * (1.0f - db);
		break;
	case BLEND_LIGHTEN:
		r = fmaxf(dr, sr);
		g = fmaxf(dg, sg);
		b = fmaxf(db, sb);
		break;
	case BLEND_DARKEN:
		r = fminf(dr, sr);
		g = fminf(dg, sg);
		b = fminf(db, sb);
		break;
	case BLEND_COLOR_BURN:
		r = (sr == 0) ? 0 : CLAMP(1.0 - ((1.0 - dr) * 1.0/sr), 0.0f, 1.0f);
		g = (sg == 0) ? 0 : CLAMP(1.0 - ((1.0 - dg) * 1.0/sg), 0.0f, 1.0f);
		b = (sb == 0) ? 0 : CLAMP(1.0 - ((1.0 - db) * 1.0/sb), 0.0f, 1.0f);
		break;
	case BLEND_COLOR_DODGE:
		r = (sr == 1.0) ? 1.0 : CLAMP(dr / (1.0 - sr), 0.0f, 1.0f);
		g = (sg == 1.0) ? 1.0 : CLAMP(dg / (1.0 - sg), 0.0f, 1.0f);
		b = (sb == 1.0) ? 1.0 : CLAMP(db / (1.0 - sb), 0.0f, 1.0f);
		break;
	case BLEND_LINEAR_BURN:
		r = CLAMP(dr + sr - 1.0, 0.0f, 1.0f);
		g = CLAMP(dg + sg - 1.0, 0.0f, 1.0f);
		b = CLAMP(db + sb - 1.0, 0.0f, 1.0f);
		break;
	case BLEND_LINEAR_DODGE:
		r = CLAMP(dr + sr, 0.0f, 1.0f);
		g = CLAMP(dg + sg, 0.0f, 1.0f);
		b = CLAMP(db + sb, 0.0f, 1.0f);
		break;
	case BLEND_COLOR_ERASE:
		/* Reduce destination color toward gray where source is similar */
		r = dr - (sr - 0.5f) * alpha;
		g = dg - (sg - 0.5f) * alpha;
		b = db - (sb - 0.5f) * alpha;
		break;
	case BLEND_MERGE:
		/* Averaging / soft merge */
		r = (dr + sr) * 0.5f;
		g = (dg + sg) * 0.5f;
		b = (db + sb) * 0.5f;
		break;
	case BLEND_SPLIT:
		/* Difference split mode */
		r = fabsf(dr - sr);
		g = fabsf(dg - sg);
		b = fabsf(db - sb);
		break;
	case BLEND_ATTRITION:
		/* Random erosion effect — darkens randomly */
		if (randf() < alpha * 0.5f) {
			r = dr * 0.5f;
			g = dg * 0.5f;
			b = db * 0.5f;
		} else {
			r = dr;
			g = dg;
			b = db;
		}
		break;
	case BLEND_GRAIN_EXTRACT:
		/* dst - src + 0.5 */
		r = CLAMP(dr - sr + 0.5f, 0.0f, 1.0f);
		g = CLAMP(dg - sg + 0.5f, 0.0f, 1.0f);
		b = CLAMP(db - sb + 0.5f, 0.0f, 1.0f);
		break;
	case BLEND_GRAIN_MERGE:
		/* dst + src - 0.5 */
		r = CLAMP(dr + sr - 0.5f, 0.0f, 1.0f);
		g = CLAMP(dg + sg - 0.5f, 0.0f, 1.0f);
		b = CLAMP(db + sb - 0.5f, 0.0f, 1.0f);
		break;
	case BLEND_DIVIDE:
		/* divide safely */
		r = CLAMP((sr <= 0.0f) ? 1.0f : dr / sr, 0.0f, 1.0f);
		g = CLAMP((sg <= 0.0f) ? 1.0f : dg / sg, 0.0f, 1.0f);
		b = CLAMP((sb <= 0.0f) ? 1.0f : db / sb, 0.0f, 1.0f);
		break;
	case BLEND_LCH_HUE:
	case BLEND_LCH_CHROMA:
	case BLEND_LCH_COLOR:
	case BLEND_LCH_LIGHTNESS: {
		RGB src_rgb = { sr, sg, sb };
		RGB dst_rgb = { dr, dg, db };
		RGB result = blend_lch_mode(dst_rgb, src_rgb, mode);
		r = result.r;
		g = result.g;
		b = result.b;
		break;
	}
	case BLEND_HSV_HUE:
	case BLEND_HSV_SATURATION:
	case BLEND_HSV_COLOR:
	case BLEND_HSV_LUMINOSITY: {
		RGB src_rgb = { sr, sg, sb };
		RGB dst_rgb = { dr, dg, db };
		RGB result = blend_hsl_mode(dst_rgb, src_rgb, mode);
		r = result.r;
		g = result.g;
		b = result.b;
		break;
	}
	case BLEND_NORMAL:
	default: /* normal */
		r = sr;
		g = sg;
		b = sb;
		break;
	}

	/* composite */
	dr = dr * (1.0f - alpha) + r * alpha;
	dg = dg * (1.0f - alpha) + g * alpha;
	db = db * (1.0f - alpha) + b * alpha;

	dst[2] = (uint8_t)(dr * 255.0f);
	dst[1] = (uint8_t)(dg * 255.0f);
	dst[0] = (uint8_t)(db * 255.0f);
}

static void
blend_images(XImage *dst, XImage *src, Monitor *m, BlendOptions *opt)
{
	if (!dst || !src || !dst->data || !src->data) {
		return;
	}

	int dst_x0 = 0, dst_y0 = 0;

	int dst_bpp = dst->bits_per_pixel / 8;
	int src_bpp = src->bits_per_pixel / 8;

	/* Determine cropped/centered region */
	int draw_w = CLAMP(src->width, 0, m->mw);
	int draw_h = CLAMP(src->height, 0, m->mh);

	/* Center the image based on monitor position */
	switch (opt->blend_position) {
	case CENTER:
		dst_x0 = m->mx + (m->mw - draw_w) / 2;
		dst_y0 = m->my + (m->mh - draw_h) / 2;
		break;
	case RELATIVE:
		if (opt->x < 0)
			dst_x0 = m->mx + m->mw - draw_w + opt->x + 1;
		else
			dst_x0 = m->mx + opt->x;
		if (opt->y < 0)
			dst_y0 = m->my + m->mh - draw_h + opt->y + 1;
		else
			dst_y0 = m->my + opt->y;
		break;
	case TILE:
		dst_x0 = m->mx;
		dst_y0 = m->my;
		draw_w = m->mw;
		draw_h = m->mh;
		break;
	}

	/* Starting position for the source window, if greater than destination */
	int src_x0 = (src->width > m->mw) ? (src->width - m->mw) / 2 : 0;
	int src_y0 = (src->height > m->mh) ? (src->height - m->mh) / 2 : 0;

	for (int y = 0; y < draw_h; y++) {
		uint8_t *dst_row = (uint8_t *)dst->data + (dst_y0 + y) * dst->bytes_per_line;
		uint8_t *src_row = (uint8_t *)src->data + (src_y0 + (y % src->height)) * src->bytes_per_line;

		for (int x = 0; x < draw_w; x++) {
			uint8_t *dst_px = dst_row + (dst_x0 + x) * dst_bpp;
			uint8_t *src_px = src_row + (src_x0 + (x % src->width)) * src_bpp;

			uint8_t sr, sg, sb, sa = 255;

			/* Handle source format (RGBA or ARGB), BGRA for imlib2 */
			switch (src_bpp) {
			case 8:
				/* Assume RGBA64 (farbfeld) */
				sr = ((src_px[0] << 8) | src_px[1]) >> 8;
				sg = ((src_px[2] << 8) | src_px[3]) >> 8;
				sb = ((src_px[4] << 8) | src_px[5]) >> 8;
				sa = ((src_px[6] << 8) | src_px[7]) >> 8;
				break;
			case 4:
				/* Assume BGRA32 (imlib) */
				sb = src_px[0];
				sg = src_px[1];
				sr = src_px[2];
				sa = src_px[3];
				break;
			default:
				sr = sg = sb = src_px[0];
				break;
			}

			blend_pixel(dst_px, sr, sg, sb, sa, opt->blend_strength, opt->blend_mode);
		}
	}
}
