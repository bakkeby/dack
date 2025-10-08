void
filter_horizontal_tear(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data) return;

	int bpp = img->bits_per_pixel / 8;
	if (bpp < 3) return;

	int stride = img->bytes_per_line;

	/* Parameters */
	int max_shift = (p->parameters[0] > 0) ? (int)p->parameters[0] : 30; /* max horizontal shift */
	double tear_prob = (p->parameters[1] > 0) ? p->parameters[1] : 0.02; /* chance per row */
	int block_size = (p->parameters[2] > 0) ? (int)p->parameters[2] : 8; /* tear height in rows */

	uint8_t *rowbuf = malloc(stride);
	if (!rowbuf) return;

	for (int y = 0; y < img->height; y++) {
		if ((rand() / (double)RAND_MAX) < tear_prob) {
			int shift = (rand() % (2 * max_shift + 1)) - max_shift;

			/* Apply to block of rows */
			for (int by = 0; by < block_size && (y + by) < img->height; by++) {
				uint8_t *row = (uint8_t*)img->data + (y + by) * stride;
				memcpy(rowbuf, row, stride);

				for (int x = 0; x < img->width; x++) {
					int srcx = x + shift;
					if (srcx < 0) srcx = 0;
					if (srcx >= img->width) srcx = img->width - 1;

					uint8_t *src = rowbuf + srcx * bpp;
					uint8_t *dst = row + x * bpp;

					dst[0] = src[0];
					dst[1] = src[1];
					dst[2] = src[2];
					if (bpp == 4) dst[3] = 0xff;
				}
			}
			y += block_size; /* skip processed block */
		}
	}

	free(rowbuf);
}
