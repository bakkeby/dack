#include <math.h>

/* Helper: clamp 0–255 */
static inline unsigned char clamp_int(int v) {
	return (v < 0) ? 0 : (v > 255 ? 255 : v);
}

/* Convert RGB → HSL */
static void rgb_to_hsl(unsigned char r, unsigned char g, unsigned char b,
                       double *h, double *s, double *l)
{
	double rf = r / 255.0;
	double gf = g / 255.0;
	double bf = b / 255.0;

	double max = fmax(rf, fmax(gf, bf));
	double min = fmin(rf, fmin(gf, bf));
	*l = (max + min) / 2.0;

	if (max == min) {
		*h = *s = 0.0;
	} else {
		double d = max - min;
		*s = (*l > 0.5) ? d / (2.0 - max - min) : d / (max + min);

		if (max == rf)
			*h = (gf - bf) / d + (gf < bf ? 6.0 : 0.0);
		else if (max == gf)
			*h = (bf - rf) / d + 2.0;
		else
			*h = (rf - gf) / d + 4.0;

		*h /= 6.0;
	}
}

/* Helper for HSL → RGB */
static double hue_to_rgb(double p, double q, double t) {
	if (t < 0.0) t += 1.0;
	if (t > 1.0) t -= 1.0;
	if (t < 1.0/6.0) return p + (q - p) * 6.0 * t;
	if (t < 1.0/2.0) return q;
	if (t < 2.0/3.0) return p + (q - p) * (2.0/3.0 - t) * 6.0;
	return p;
}

/* Convert HSL → RGB */
static void hsl_to_rgb(double h, double s, double l,
                       unsigned char *r, unsigned char *g, unsigned char *b)
{
	double rf, gf, bf;

	if (s == 0.0) {
		rf = gf = bf = l; // achromatic
	} else {
		double q = (l < 0.5) ? l * (1.0 + s) : l + s - l * s;
		double p = 2.0 * l - q;
		rf = hue_to_rgb(p, q, h + 1.0/3.0);
		gf = hue_to_rgb(p, q, h);
		bf = hue_to_rgb(p, q, h - 1.0/3.0);
	}

	*r = clamp_int((int)(rf * 255.0));
	*g = clamp_int((int)(gf * 255.0));
	*b = clamp_int((int)(bf * 255.0));
}

/* ----------------------------------------------------------------------
 * filter_hue
 * parameters[0] = hue shift in degrees (0–360)
 * ---------------------------------------------------------------------- */
void
filter_hue(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data || !lock || !lock->m)
		return;

	Monitor *m;
	/* parameters[0] = hue shift in degrees */
	double hue_shift = p->parameters[0] / 360.0; /* normalize */
	int bpp = (img->bits_per_pixel + 7) / 8;
	int stride = img->bytes_per_line;

	for (m = lock->m; m; m = m->next) {
		for (int y = m->my; y < m->my + m->mh; y++) {
			unsigned char *row = (unsigned char *)img->data + y * stride;
			for (int x = m->mx; x < m->mx + m->mw; x++) {
				unsigned char *px = row + x * bpp;

				unsigned char r = px[2], g = px[1], b = px[0];
				double h, s, l;

				rgb_to_hsl(r, g, b, &h, &s, &l);

				h += hue_shift;
				if (h > 1.0) h -= 1.0;
				if (h < 0.0) h += 1.0;

				hsl_to_rgb(h, s, l, &r, &g, &b);

				px[2] = r;
				px[1] = g;
				px[0] = b;
			}
		}
	}
}
