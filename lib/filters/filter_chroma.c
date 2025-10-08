void
filter_chroma_crawl(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data) return;
	int bpp = img->bits_per_pixel / 8;
	if (bpp < 3) return;
	int stride = img->bytes_per_line;

	double freq = p->parameters[0];   /* frequency of oscillation */
	double amp  = p->parameters[1];   /* strength of modulation */

	for (int y = 0; y < img->height; y++) {
		uint8_t *row = (uint8_t*)img->data + y * stride;

		for (int x = 0; x < img->width; x++) {
			uint8_t *dst = row + x * bpp;
			double mod = 1.0 + amp * sin(freq * x + y * 0.05);

			for (int c = 0; c < 3; c++) {
				int v = (int)(dst[c] * mod);
				if (v < 0) v = 0;
				if (v > 255) v = 255;
				dst[c] = (uint8_t)v;
			}
		}
	}
}

void
filter_chroma_drift(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data) return;
	int bpp = img->bits_per_pixel / 8;
	if (bpp < 3) return;
	int stride = img->bytes_per_line;

	int shift_r = -1 * (int)p->parameters[0]; /* shift for red   */
	int shift_g = -1 * (int)p->parameters[1]; /* shift for green */
	int shift_b = -1 * (int)p->parameters[2]; /* shift for blue  */
	int vertical = (int)p->parameters[3]; /* 0 = horizontal, 1 = vertical */

	if (vertical) {
		/* -------- Vertical Drift -------- */
		uint8_t *copybuf = malloc(stride * img->height);
		if (!copybuf) return;
		memcpy(copybuf, img->data, stride * img->height);

		for (int y = 0; y < img->height; y++) {
			for (int x = 0; x < img->width; x++) {
				int sy_r = y + shift_r;
				int sy_g = y + shift_g;
				int sy_b = y + shift_b;

				if (sy_r < 0) { sy_r = 0; }; if (sy_r >= img->height) { sy_r = img->height - 1; };
				if (sy_g < 0) { sy_g = 0; }; if (sy_g >= img->height) { sy_g = img->height - 1; };
				if (sy_b < 0) { sy_b = 0; }; if (sy_b >= img->height) { sy_b = img->height - 1; };

				uint8_t *dst   = (uint8_t*)img->data + y * stride + x * bpp;
				uint8_t *src_r = copybuf + sy_r * stride + x * bpp;
				uint8_t *src_g = copybuf + sy_g * stride + x * bpp;
				uint8_t *src_b = copybuf + sy_b * stride + x * bpp;

				dst[2] = src_r[2]; /* red   */
				dst[1] = src_g[1]; /* green */
				dst[0] = src_b[0]; /* blue  */
			}
		}

		free(copybuf);
	} else {
		/* -------- Horizontal Drift -------- */
		uint8_t *rowbuf = malloc(stride);
		if (!rowbuf) return;

		for (int y = 0; y < img->height; y++) {
			uint8_t *row = (uint8_t*)img->data + y * stride;
			memcpy(rowbuf, row, stride);

			for (int x = 0; x < img->width; x++) {
				int sx_r = x + shift_r;
				int sx_g = x + shift_g;
				int sx_b = x + shift_b;

				if (sx_r < 0) { sx_r = 0; }; if (sx_r >= img->width) { sx_r = img->width - 1; };
				if (sx_g < 0) { sx_g = 0; }; if (sx_g >= img->width) { sx_g = img->width - 1; };
				if (sx_b < 0) { sx_b = 0; }; if (sx_b >= img->width) { sx_b = img->width - 1; };

				uint8_t *dst = row + x * bpp;
				dst[2] = rowbuf[sx_r * bpp + 2]; /* red   */
				dst[1] = rowbuf[sx_g * bpp + 1]; /* green */
				dst[0] = rowbuf[sx_b * bpp + 0]; /* blue  */
			}
		}

		free(rowbuf);
	}
}
