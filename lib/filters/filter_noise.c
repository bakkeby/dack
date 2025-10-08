void
filter_noise(XImage *img, EffectParams *p, struct lock *lock)
{
	double amount = p->parameters[0];

	if (!img) return;
	if (amount <= 0.0) return;          /* nothing to do */
	if (amount > 1.0) amount = 1.0;     /* clamp */

	/* Bytes per pixel – works for 24‑bpp (3) and 32‑bpp (4). */
	int bpp = img->bits_per_pixel / 8;
	if (bpp == 0) bpp = 4;               /* safety fallback */

	/* Total number of pixels in the image */
	const long total_pixels = (long)img->width * (long)img->height;

	/* How many pixels we will corrupt */
	long n_noise = (long)(total_pixels * amount + 0.5);
	if (n_noise == 0) n_noise = 1;      /* guarantee at least one */

	for (long i = 0; i < n_noise; ++i) {
		/* Pick a random pixel coordinate */
		int x = rand() % img->width;
		int y = rand() % img->height;
		/* Build a random colour value.
		   For 24‑bpp we generate 3 bytes (RGB).  For 32‑bpp we also
		   generate an unused alpha byte – most X servers ignore it. */
		uint32_t rnd = ((uint32_t)rand() << 16) | (uint32_t)rand();
		unsigned char *dst = (unsigned char *)img->data +
							 y * img->bytes_per_line + x * bpp;

		/* Copy the low‑order bytes into the pixel */
		for (int k = 0; k < bpp; ++k) {
			dst[k] = (unsigned char)(rnd & 0xFF);
			rnd >>= 8;
		}
	}
}

void
filter_soft_noise(XImage *img, EffectParams *p, struct lock *lock)
{
	double amount = p->parameters[0];
	double strength = p->parameters[1];
	int blur_radius = (int)p->parameters[2];

	if (!img) return;
	if (amount <= 0.0) return;
	if (strength <= 0.0) strength = 0.01;   /* avoid division by zero */
	if (strength > 1.0) strength = 1.0;
	if (blur_radius < 0) blur_radius = 0;

	/* ---------------------------------------------------------
	   Determine bytes‑per‑pixel (works for 24‑ and 32‑bpp)
	   --------------------------------------------------------- */
	int bpp = img->bits_per_pixel / 8;
	if (bpp == 0) bpp = 4;                 /* safety fallback */

	const long total_pixels = (long)img->width * (long)img->height;
	long n_noise = (long)(total_pixels * amount + 0.5);
	if (n_noise == 0) n_noise = 1;         /* at least one pixel */

	/* ---------------------------------------------------------
	   Add noise – each affected pixel receives a colour
	   that is a weighted blend of its original value and a
	   random offset limited by `strength`.
	   --------------------------------------------------------- */
	for (long i = 0; i < n_noise; ++i) {
		int x = rand() % img->width;
		int y = rand() % img->height;

		unsigned char *pix = (unsigned char *)img->data +
							 y * img->bytes_per_line + x * bpp;

		/* Original colour components (0‑255) */
		unsigned char orig[4];
		for (int c = 0; c < bpp; ++c) orig[c] = pix[c];

		/* Random offset limited to ±strength*255 */
		int max_delta = (int)(strength * 255.0);
		for (int c = 0; c < bpp; ++c) {
			int delta = (rand() % (2 * max_delta + 1)) - max_delta;
			int val   = (int)orig[c] + delta;
			if (val <   0) val =   0;
			if (val > 255) val = 255;
			pix[c] = (unsigned char)val;
		}
	}

	/* ---------------------------------------------------------
	   Optional box‑blur to smooth the high‑frequency spikes.
	   We implement a separable blur (horizontal then vertical)
	   because it is O(width·height·radius) instead of O(N²).
	   --------------------------------------------------------- */
	if (blur_radius > 0) {
		/* Allocate a temporary buffer the size of the image */
		unsigned char *tmp = malloc(img->height * img->bytes_per_line);
		if (!tmp) return;   /* OOM – skip blur */

		/* ----- Horizontal pass ----- */
		for (int y = 0; y < img->height; ++y) {
			unsigned char *src = (unsigned char *)img->data + y * img->bytes_per_line;
			unsigned char *dst = tmp + y * img->bytes_per_line;

			for (int x = 0; x < img->width; ++x) {
				int sum[4] = {0,0,0,0};
				int cnt = 0;
				for (int k = -blur_radius; k <= blur_radius; ++k) {
					int xx = x + k;
					if (xx < 0 || xx >= img->width) continue;
					unsigned char *p = src + xx * bpp;
					for (int c = 0; c < bpp; ++c) sum[c] += p[c];
					++cnt;
				}
				unsigned char *out = dst + x * bpp;
				for (int c = 0; c < bpp; ++c) out[c] = (unsigned char)(sum[c] / cnt);
			}
		}

		/* ----- Vertical pass (write back into img->data) ----- */
		for (int y = 0; y < img->height; ++y) {
			unsigned char *dst = (unsigned char *)img->data + y * img->bytes_per_line;
			for (int x = 0; x < img->width; ++x) {
				int sum[4] = {0,0,0,0};
				int cnt = 0;
				for (int k = -blur_radius; k <= blur_radius; ++k) {
					int yy = y + k;
					if (yy < 0 || yy >= img->height) continue;
					unsigned char *p = tmp + yy * img->bytes_per_line + x * bpp;
					for (int c = 0; c < bpp; ++c) sum[c] += p[c];
					++cnt;
				}
				unsigned char *out = dst + x * bpp;
				for (int c = 0; c < bpp; ++c) out[c] = (unsigned char)(sum[c] / cnt);
			}
		}

		free(tmp);
	}
}
