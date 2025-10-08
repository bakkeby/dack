void
filter_wave_distortion(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data) return;
	int bpp = img->bits_per_pixel / 8;
	if (bpp < 3) return;
	int stride = img->bytes_per_line;

	double amplitude = p->parameters[0]; /* max horizontal shift */
	double frequency = p->parameters[1]; /* wave frequency */
	double phase = p->parameters[2];     /* phase offset */

	uint8_t *rowbuf = malloc(stride);
	if (!rowbuf) return;

	for (int y = 0; y < img->height; y++) {
		uint8_t *row = (uint8_t*)img->data + y * stride;
		memcpy(rowbuf, row, stride);

		int shift = (int)(amplitude * sin(frequency * y + phase));

		for (int x = 0; x < img->width; x++) {
			int sx = x + shift;
			if (sx < 0) sx = 0;
			if (sx >= img->width) sx = img->width - 1;

			uint8_t *src = rowbuf + sx * bpp;
			uint8_t *dst = row + x * bpp;

			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			if (bpp == 4) dst[3] = 0xff;
		}
	}
	free(rowbuf);
}
