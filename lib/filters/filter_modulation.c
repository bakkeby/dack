void
filter_modulation_glitch(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data) return;

	int intensity   = (p->parameters[0] > 0) ? (int)p->parameters[0] : 10;   /* max horizontal jitter in px */
	double color_mod = (p->parameters[1] > 0) ? p->parameters[1] : 0.2;      /* color modulation strength (0..1) */
	double noise_prob = (p->parameters[2] > 0) ? p->parameters[2] : 0.01;    /* chance of random noise per pixel */
	int chroma_sep   = (p->parameters[3] > 0.5);                             /* enable channel separation if nonzero */

	int bpp = img->bits_per_pixel / 8;
	if (bpp < 3) return;

	int stride = img->bytes_per_line;
	uint8_t *rowbuf = malloc(stride);

	for (int y = 0; y < img->height; y++) {
		uint8_t *row = (uint8_t*)img->data + y * stride;
		memcpy(rowbuf, row, stride);

		/* Random horizontal jitter */
		int shift = (rand() % (2 * intensity + 1)) - intensity;

		for (int x = 0; x < img->width; x++) {
			int srcx = x + shift;
			if (srcx < 0) srcx = 0;
			if (srcx >= img->width) srcx = img->width - 1;

			uint8_t *src = rowbuf + srcx * bpp;
			uint8_t *dst = row + x * bpp;

			/* Copy base color */
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			if (bpp == 4) dst[3] = 0xff;

			/* Random color modulation */
			if ((rand() % 100) < (color_mod * 100)) {
				dst[0] = (uint8_t)((int)dst[0] + (rand() % 51 - 25)); /* blue shift */
				dst[1] = (uint8_t)((int)dst[1] + (rand() % 51 - 25)); /* green shift */
				dst[2] = (uint8_t)((int)dst[2] + (rand() % 51 - 25)); /* red shift */
			}

			/* Sprinkle noise */
			if ((rand() / (double)RAND_MAX) < noise_prob) {
				dst[0] = rand() % 256;
				dst[1] = rand() % 256;
				dst[2] = rand() % 256;
			}

			/* Optional RGB channel separation */
			if (chroma_sep) {
				int rshift = (rand() % (2 * intensity + 1)) - intensity;
				int gshift = (rand() % (2 * intensity + 1)) - intensity;
				int bshift = (rand() % (2 * intensity + 1)) - intensity;

				int rsrcx = x + rshift;
				int gsrcx = x + gshift;
				int bsrcx = x + bshift;
				if (rsrcx < 0) rsrcx = 0;
				if (rsrcx >= img->width) rsrcx = img->width - 1;
				if (gsrcx < 0) gsrcx = 0;
				if (gsrcx >= img->width) gsrcx = img->width - 1;
				if (bsrcx < 0) bsrcx = 0;
				if (bsrcx >= img->width) bsrcx = img->width - 1;

				dst[2] = rowbuf[rsrcx * bpp + 2]; /* red from shifted location */
				dst[1] = rowbuf[gsrcx * bpp + 1]; /* red from shifted location */
				dst[0] = rowbuf[bsrcx * bpp + 0]; /* blue from shifted location */
			}
		}
	}

	free(rowbuf);
}
