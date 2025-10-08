/* ----------------------------------------------------------------------
 * filter_brightness
 * parameters[0] = brightness multiplier
 *                 < 1.0 = darken
 *                 > 1.0 = brighten
 *                 1.0 = no change
 * ---------------------------------------------------------------------- */
void
filter_brightness(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data || !lock || !lock->m)
		return;

	Monitor *m;
	double offset = p->parameters[0]; /* range: -255..255 */
	int bpp = (img->bits_per_pixel + 7) / 8;
	int stride = img->bytes_per_line;

	for (m = lock->m; m; m = m->next) {
		for (int y = m->my; y < m->my + m->mh; y++) {
			unsigned char *row = (unsigned char *)img->data + y * stride;
			for (int x = m->mx; x < m->mx + m->mw; x++) {
				unsigned char *px = row + x * bpp;
				for (int c = 0; c < 3; c++) {
					int v = px[c] + offset;
					if (v < 0) v = 0;
					if (v > 255) v = 255;
					px[c] = v;
				}
			}
		}
	}
}
