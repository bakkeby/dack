void
filter_flip(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data || !lock || !lock->m)
		return;

	Monitor *m;
	int y;
	int flip_h = (int)p->parameters[0];
	int flip_v = (int)p->parameters[1];

	if (!flip_h && !flip_v)
		return; /* nothing to do */

	int bpp    = (img->bits_per_pixel + 7) / 8;
	int stride = img->bytes_per_line;

	/* Work through each monitor region */
	for (m = lock->m; m; m = m->next) {
		int mx = m->mx;
		int my = m->my;
		int mw = m->mw;
		int mh = m->mh;

		/* Temporary buffer for the monitor image */
		size_t bufsize = (size_t)mh * mw * bpp;
		unsigned char *copy = malloc(bufsize);
		if (!copy) return;

		/* Copy monitor pixels into temp buffer */
		for (y = 0; y < mh; y++) {
			unsigned char *src = (unsigned char *)img->data +
			                     (my + y) * stride + mx * bpp;
			memcpy(copy + y * mw * bpp, src, mw * bpp);
		}

		/* Apply flips into the original image */
		for (y = 0; y < mh; y++) {
			for (int x = 0; x < mw; x++) {
				int src_x = flip_h ? (mw - 1 - x) : x;
				int src_y = flip_v ? (mh - 1 - y) : y;

				unsigned char *src = copy + (src_y * mw + src_x) * bpp;
				unsigned char *dst = (unsigned char *)img->data +
				                     (my + y) * stride + (mx + x) * bpp;
				memcpy(dst, src, bpp);
			}
		}

		free(copy);
	}
}
