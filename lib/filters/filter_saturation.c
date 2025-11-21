/* ----------------------------------------------------------------------
 * filter_saturation
 * parameters[0] = saturation multiplier
 *                 0.0 = grayscale
 *                 1.0 = no change
 *                 >1.0 = boost saturation
 * ---------------------------------------------------------------------- */
void
filter_saturation(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data || !lock || !lock->m)
		return;

	Monitor *m;
	double factor = p->parameters[0]; /* 0.0=gray, 1.0=no change, >1=boost */
	if (factor < 0.0) factor = 0.0;

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

				/* Compute perceived luminance (rec. 601 luma) */
				double gray =
				    0.299 * rgb.r +
				    0.587 * rgb.g +
				    0.114 * rgb.b;

				/* Interpolate away from gray */
				double r = gray + (rgb.r - gray) * factor;
				double g = gray + (rgb.g - gray) * factor;
				double b = gray + (rgb.b - gray) * factor;

				px[2] = CLAMP255(r);
				px[1] = CLAMP255(g);
				px[0] = CLAMP255(b);
			}
		}
	}
}
