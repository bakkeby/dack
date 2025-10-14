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

				RGB rgb = {
					.r = px[2],
					.g = px[1],
					.b = px[0],
				};

				HSL hsl = rgb_to_hsl(rgb);

				hsl.h += hue_shift;
				if (hsl.h > 1.0) hsl.h -= 1.0;
				if (hsl.h < 0.0) hsl.h += 1.0;

				rgb = hsl_to_rgb(hsl);

				px[2] = rgb.r;
				px[1] = rgb.g;
				px[0] = rgb.b;
			}
		}
	}
}
