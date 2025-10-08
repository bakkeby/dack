void
filter_color_bleed(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data) return;
	int bpp = img->bits_per_pixel / 8;
	if (bpp < 3) return;
	int stride = img->bytes_per_line;

	int radius = (int)p->parameters[0]; /* bleed radius */
	double strength = p->parameters[1]; /* how much to blend */

	uint8_t *rowbuf = malloc(stride);
	if (!rowbuf) return;

	for (int y = 0; y < img->height; y++) {
		uint8_t *row = (uint8_t*)img->data + y * stride;
		memcpy(rowbuf, row, stride);

		for (int x = 0; x < img->width; x++) {
			int accum[3] = {0,0,0};
			int count = 0;

			for (int dx = -radius; dx <= radius; dx++) {
				int sx = x + dx;
				if (sx < 0 || sx >= img->width) continue;
				uint8_t *src = rowbuf + sx * bpp;
				for (int c = 0; c < 3; c++) accum[c] += src[c];
				count++;
			}

			uint8_t *dst = row + x * bpp;
			for (int c = 0; c < 3; c++) {
				int avg = accum[c] / count;
				dst[c] = (uint8_t)((1.0 - strength) * dst[c] + strength * avg);
			}
		}
	}
	free(rowbuf);
}
