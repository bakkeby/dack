void
filter_blur(XImage *img, EffectParams *p, struct lock *lock)
{
	int radius = (int)p->parameters[0];

	int width  = img->width;
	int height = img->height;
	int bpp    = img->bits_per_pixel / 8;  // 4
	int stride = img->bytes_per_line;

	// allocate temporary buffer
	uint8_t *tmp = malloc(stride * height);
	memcpy(tmp, img->data, stride * height);  // start with original

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int rsum = 0, gsum = 0, bsum = 0, count = 0;

			for (int dy = -radius; dy <= radius; dy++) {
				int yy = y + dy;
				if (yy < 0 || yy >= height) continue;

				uint8_t *row = (uint8_t *)img->data + yy * stride;
				for (int dx = -radius; dx <= radius; dx++) {
					int xx = x + dx;
					if (xx < 0 || xx >= width) continue;

					uint8_t *p = row + xx * bpp;
					bsum += p[0];  // B
					gsum += p[1];  // G
					rsum += p[2];  // R
					count++;
				}
			}

			// write averaged value to temporary buffer
			uint8_t *dst = tmp + y * stride + x * bpp;
			dst[0] = bsum / count;
			dst[1] = gsum / count;
			dst[2] = rsum / count;
			if (bpp == 4) dst[3] = 0;  // preserve alpha/padding
		}
	}

	// copy blurred buffer back to image
	memcpy(img->data, tmp, stride * height);
	free(tmp);
}

/* -------------------------------------------------------------
   Helper: fetch a pixel from a buffer (no bounds check)
   ------------------------------------------------------------- */
static inline void get_pixel(const unsigned char *buf,
							 int stride, int bpp,
							 int x, int y,
							 unsigned char out[4])
{
	const unsigned char *p = buf + y * stride + x * bpp;
	for (int i = 0; i < bpp; ++i) out[i] = p[i];
}

/* Helper: write a pixel into a buffer */
static inline void set_pixel(unsigned char *buf,
							 int stride, int bpp,
							 int x, int y,
							 const unsigned char in[4])
{
	unsigned char *p = buf + y * stride + x * bpp;
	for (int i = 0; i < bpp; ++i) p[i] = in[i];
}

/* -------------------------------------------------------------
   dual kawase blur
   ------------------------------------------------------------- */
void
filter_dual_kawase_blur(XImage *img, EffectParams *p, struct lock *lock)
{
	int iterations = (int)p->parameters[0];
	int radius = (int)p->parameters[1];

	if (!img || iterations <= 0 || radius < 1)
		return;

	/* ---------------------------------------------------------
	   Determine bytes‑per‑pixel (24‑bpp => 3, 32‑bpp => 4)
	   --------------------------------------------------------- */
	int bpp = img->bits_per_pixel / 8;
	if (bpp < 3)
		return;  /* unsupported depth */

	/* ---------------------------------------------------------
	   Allocate two auxiliary buffers that will hold the
	   intermediate down‑sampled image.  We allocate the *largest*
	   possible size (original width/height) for simplicity and
	   reuse the same memory for every iteration.
	   --------------------------------------------------------- */
	size_t buf_sz = (size_t)img->height * img->bytes_per_line;
	unsigned char *tmp0 = malloc(buf_sz);
	unsigned char *tmp1 = malloc(buf_sz);

	if (!tmp0 || !tmp1) {
		free(tmp0);
		free(tmp1);
		return;
	}

	/* ---------------------------------------------------------
	   Copy the original image into tmp0 – this becomes our
	   working source for the first down‑sample pass.
	   --------------------------------------------------------- */
	memcpy(tmp0, img->data, buf_sz);

	int src_w = img->width;
	int src_h = img->height;
	int src_stride = img->bytes_per_line;

	for (int iter = 0; iter < iterations; ++iter) {
		/* ---------- Down‑sample pass (tmp0 -> tmp1) ---------- */
		int dst_w = src_w > 1 ? src_w / 2 : 1;
		int dst_h = src_h > 1 ? src_h / 2 : 1;
		int dst_stride = src_stride;   /* keep same stride for simplicity */

		for (int y = 0; y < dst_h; ++y) {
			for (int x = 0; x < dst_w; ++x) {
				/* Source coordinates in the higher‑res buffer */
				int sx = x * 2;
				int sy = y * 2;

				/* Accumulate centre + 4 orthogonal neighbours */
				unsigned int sum[4] = {0,0,0,0};
				unsigned char pix[4];

				/* centre */
				get_pixel(tmp0, src_stride, bpp, sx, sy, pix);
				for (int c = 0; c < bpp; ++c) sum[c] += pix[c];

				/* left */
				if (sx - radius >= 0) {
					get_pixel(tmp0, src_stride, bpp, sx - radius, sy, pix);
					for (int c = 0; c < bpp; ++c) sum[c] += pix[c];
				}

				/* right */
				if (sx + radius < src_w) {
					get_pixel(tmp0, src_stride, bpp, sx + radius, sy, pix);
					for (int c = 0; c < bpp; ++c) sum[c] += pix[c];
				}

				/* top */
				if (sy - radius >= 0) {
					get_pixel(tmp0, src_stride, bpp, sx, sy - radius, pix);
					for (int c = 0; c < bpp; ++c) sum[c] += pix[c];
				}

				/* bottom */
				if (sy + radius < src_h) {
					get_pixel(tmp0, src_stride, bpp, sx, sy + radius, pix);
					for (int c = 0; c < bpp; ++c) sum[c] += pix[c];
				}

				/* Average (5 taps max) */
				unsigned char out[4];
				for (int c = 0; c < bpp; ++c) out[c] = (unsigned char)(sum[c] / 5);
				set_pixel(tmp1, dst_stride, bpp, x, y, out);
			}
		}

		/* ---------- Up‑sample pass (tmp1 -> tmp0) ---------- */
		/* Now tmp1 holds the down‑scaled image (dst_w × dst_h). */
		/* We up‑sample back to src_w × src_h. */
		for (int y = 0; y < src_h; ++y) {
			for (int x = 0; x < src_w; ++x) {
				/* Corresponding coordinate in the low‑res buffer */
				int lx = x / 2;
				int ly = y / 2;

				/* Accumulate centre + 4 neighbours in low‑res space */
				unsigned int sum[4] = {0,0,0,0};
				unsigned char pix[4];

				/* centre */
				get_pixel(tmp1, dst_stride, bpp, lx, ly, pix);
				for (int c = 0; c < bpp; ++c) sum[c] += pix[c];

				/* left */
				if (lx - radius >= 0) {
					get_pixel(tmp1, dst_stride, bpp, lx - radius, ly, pix);
					for (int c = 0; c < bpp; ++c) sum[c] += pix[c];
				}

				/* right */
				if (lx + radius < dst_w) {
					get_pixel(tmp1, dst_stride, bpp, lx + radius, ly, pix);
					for (int c = 0; c < bpp; ++c) sum[c] += pix[c];
				}

				/* top */
				if (ly - radius >= 0) {
					get_pixel(tmp1, dst_stride, bpp, lx, ly - radius, pix);
					for (int c = 0; c < bpp; ++c) sum[c] += pix[c];
				}

				/* bottom */
				if (ly + radius < dst_h) {
					get_pixel(tmp1, dst_stride, bpp, lx, ly + radius, pix);
					for (int c = 0; c < bpp; ++c) sum[c] += pix[c];
				}

				unsigned char out[4];
				for (int c = 0; c < bpp; ++c) out[c] = (unsigned char)(sum[c] / 5);
				set_pixel(tmp0, src_stride, bpp, x, y, out);
			}
		}

		/* Reduce the radius a little after each iteration – this
		   mimics the classic Dual‑Kawase cascade and prevents
		   overshooting. */
		if (radius > 1) radius--;
	}

	/* ---------------------------------------------------------
	   Copy the final blurred buffer back into the XImage.
	   --------------------------------------------------------- */
	memcpy(img->data, tmp0, buf_sz);

	free(tmp0);
	free(tmp1);
	return;
}
