/* Keep track of persistent distortion area */
typedef struct {
	int y;          /* starting row of VHS band */
	int initialized;
} VHSRegion;

void
filter_vhs_warp_chroma(XImage *img, EffectParams *p, struct lock *lock)
{
	static VHSRegion region = { .initialized = 0 };

	if (!img || !img->data)
		return;

	int shift_r, shift_g, shift_b;

	int bpp = img->bits_per_pixel / 8;
	int stride = img->bytes_per_line;

	if (bpp < 3) return; /* need RGB */

	int height = (int)p->parameters[0];
	int max_shift = (int)p->parameters[1];
	int min_length = (int)p->parameters[2];
	int chroma_shift = (p->parameters[3] > 0.5);

	/* Pick region once and persist across calls */
	if (!region.initialized) {
		region.y = rand() % (img->height - height);
		region.initialized = 1;
	} else {
		/* Allow region area to drift */
		int y_shift = 2 * height * rand() % 100;
		if (rand() % 100 > 50)
			y_shift *= -1;
		region.y += y_shift;
		if (region.y > img->height - height)
			region.y = img->height - height;
		if (region.y < 0)
			region.y = 0;
	}

	uint8_t *rowbuf = malloc(stride);

	for (int y = region.y; y < region.y + height; y++) {
		uint8_t *row = (uint8_t *)img->data + y * stride;

		int s = ((rand() % 100) * (img->width - min_length))/100;
		int e = s + ((rand() % 100) * (img->width - s))/100;

		/* 70% chance this row gets warped */
		if (rand() % 100 < 70) {
			/* copy original row */
			memcpy(rowbuf, row, stride);

			/* independent shifts for R, G, B */
			if (chroma_shift) {
				shift_r = (rand() % (2 * max_shift + 1)) - max_shift;
				shift_g = (rand() % (2 * max_shift + 1)) - max_shift;
				shift_b = (rand() % (2 * max_shift + 1)) - max_shift;
			} else {
				shift_r = shift_g = shift_b = (rand() % (2 * max_shift + 1)) - max_shift;
			}

			for (int x = s; x < e; x++) {
				int idx = x * bpp;

				/* compute shifted indices (clamp to row bounds) */
				int xr = x + shift_r;
				int xg = x + shift_g;
				int xb = x + shift_b;

				if (xr < 0) xr = 0; else if (xr >= img->width) xr = img->width - 1;
				if (xg < 0) xg = 0; else if (xg >= img->width) xg = img->width - 1;
				if (xb < 0) xb = 0; else if (xb >= img->width) xb = img->width - 1;

				/* copy each channel separately */
				row[idx + 0] = rowbuf[xb * bpp + 0]; /* B */
				row[idx + 1] = rowbuf[xg * bpp + 1]; /* G */
				row[idx + 2] = rowbuf[xr * bpp + 2]; /* R */
				if (bpp == 4) row[idx + 3] = 0xff;   /* Alpha (opaque) */
			}
		} else {
			/* sprinkle static dots */
			for (int x = s; x < e; x++) {
				if (rand() % 1000 == 0) { /* rare white specks */
					int idx = x * bpp;
					row[idx + 0] = 150;
					row[idx + 1] = 150;
					row[idx + 2] = 150;
					if (bpp == 4) row[idx + 3] = 0xff;
				}
			}
		}
	}

	free(rowbuf);
}

void
filter_vhs_jitter(XImage *img, EffectParams *p, struct lock *lock)
{
	int max_offset = (int)p->parameters[0];
	int height = img->height;
	int stride = img->bytes_per_line;
	uint8_t *data = (uint8_t *)img->data;

	uint8_t *tmp = malloc(stride * height);
	memcpy(tmp, data, stride * height);

	for (int y = 0; y < height; y++) {
		int offset = (rand() % (2*max_offset + 1)) - max_offset;
		int yy = y + offset;
		if (yy < 0) yy = 0;
		if (yy >= height) yy = height - 1;

		memcpy(data + y * stride, tmp + yy * stride, stride);
	}
	free(tmp);
}
