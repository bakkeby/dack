void
filter_ghosting(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data) return;
	int bpp = img->bits_per_pixel / 8;
	if (bpp < 3) return;
	int stride = img->bytes_per_line;

	int shift = (int)p->parameters[0];      /* ghost offset in pixels */
	double alpha = p->parameters[1];        /* blend factor, 0–1 */
	if (alpha < 0) alpha = 0;
	if (alpha > 1) alpha = 1;

	uint8_t *rowbuf = malloc(stride);
	if (!rowbuf) return;

	for (int y = 0; y < img->height; y++) {
		uint8_t *row = (uint8_t*)img->data + y * stride;
		memcpy(rowbuf, row, stride);

		for (int x = 0; x < img->width; x++) {
			int sx = x + shift;
			if (sx < 0) sx = 0;
			if (sx >= img->width) sx = img->width - 1;

			uint8_t *src = rowbuf + sx * bpp;
			uint8_t *dst = row + x * bpp;

			for (int c = 0; c < 3; c++) {
				dst[c] = (uint8_t)((1.0 - alpha) * dst[c] + alpha * src[c]);
			}
		}
	}
	free(rowbuf);
}
