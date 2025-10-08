/* ----------------------------------------------------------------------
 * filter_contrast
 * parameters[0] = contrast multiplier
 *                 < 1.0 = lower contrast
 *                 > 1.0 = higher contrast
 *                 1.0 = no change
 * ---------------------------------------------------------------------- */
void
filter_contrast(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data || !lock || !lock->m)
		return;

	Monitor *m;
	double factor = p->parameters[0]; /* e.g. 1.0=no change, >1.0=more contrast */
	int bpp = (img->bits_per_pixel + 7) / 8;
	int stride = img->bytes_per_line;

	for (m = lock->m; m; m = m->next) {
		for (int y = m->my; y < m->my + m->mh; y++) {
			unsigned char *row = (unsigned char *)img->data + y * stride;
			for (int x = m->mx; x < m->mx + m->mw; x++) {
				unsigned char *px = row + x * bpp;
				for (int c = 0; c < 3; c++) {
					int v = (int)(((px[c] - 128) * factor) + 128);
					if (v < 0) v = 0;
					if (v > 255) v = 255;
					px[c] = v;
				}
			}
		}
	}
}
