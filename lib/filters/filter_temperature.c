/* Converts a Kelvin temperature (1000K–40000K) to an RGB scale factor (0–1) */
static void
kelvin_to_rgb(double kelvin, double *r, double *g, double *b)
{
	kelvin = kelvin / 100.0;

	if (kelvin <= 66) {
		*r = 1.0;
		*g = kelvin;
		*g = 0.39008157876901960784 * log(*g) - 0.63184144378862745098;
		if (kelvin <= 19) {
			*b = 0.0;
		} else {
			*b = kelvin - 10;
			*b = 0.54320678911019607843 * log(*b) - 1.19625408914;
		}
	} else {
		*r = kelvin - 60;
		*r = 1.29293618606274509804 * pow(*r, -0.1332047592);
		*g = kelvin - 60;
		*g = 1.12989086089529411765 * pow(*g, -0.0755148492);
		*b = 1.0;
	}

	*r = CLAMP(*r, 0, 1);
	*g = CLAMP(*g, 0, 1);
	*b = CLAMP(*b, 0, 1);
}

void
filter_temperature(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data || !lock) return;

	int bpp = (img->bits_per_pixel + 7) / 8;
	if (bpp < 3) return;
	Monitor *m;
	int x, y;
	int stride = img->bytes_per_line;

	/* Resolve parameters */
	double kelvin = (p->parameters[0] > 0 ? p->parameters[0] : 6500);
	if (kelvin < 1000) kelvin = 1000;
	if (kelvin > 40000) kelvin = 40000;

	double strength = p->parameters[1];
	if (strength < 0.0) strength = 0.0;
	if (strength > 1.0) strength = 1.0;

	/* RGB multipliers for the target temperature */
	double r_mul, g_mul, b_mul;
	kelvin_to_rgb(kelvin, &r_mul, &g_mul, &b_mul);

	/* Walk over each monitor region */
	for (m = lock->m; m; m = m->next) {
		for (y = m->my; y < m->my + m->mh; y++) {
			uint8_t *row = (uint8_t*)img->data + y * stride + m->mx * bpp;
			for (x = 0; x < m->mw; x++) {
				uint8_t *px = row + x * bpp;

				/* Original values */
				double b = px[0];
				double g = px[1];
				double r = px[2];

				/* Temperature-scaled */
				double r_t = r * r_mul;
				double g_t = g * g_mul;
				double b_t = b * b_mul;

				/* Blend original ↔ adjusted */
				px[2] = (uint8_t)(r + (r_t - r) * strength);
				px[1] = (uint8_t)(g + (g_t - g) * strength);
				px[0] = (uint8_t)(b + (b_t - b) * strength);
			}
		}
	}
}
