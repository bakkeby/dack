/* Drop shadow filter.
 *
 * Parameters (p->parameters):
 *   0 = opacity (0..1)
 *   1 = angle in degrees
 *   2 = distance in pixels
 *   3 = size (blur radius, px)
 *   4 = noise (0..1)
 *   5 = blend_mode index (int)
 *   6 = knockout (0/1)
 *   7 = reserved
 *
 * Optional string parameter 0: "#RRGGBB" tint color (default black)
 */

/* parse "#RRGGBB" (hex) to normalized RGB [0..1], returns 0 on success */
static int parse_hex_color(const char *s, float *r, float *g, float *b) {
	if (!s) return -1;
	/* accept optionally leading '#' */
	if (s[0] == '#') s++;
	if (strlen(s) != 6) return -1;
	char tmp[3] = {0};
	unsigned int rv = 0, gv = 0, bv = 0;
	tmp[0] = s[0]; tmp[1] = s[1]; rv = (unsigned)strtoul(tmp, NULL, 16);
	tmp[0] = s[2]; tmp[1] = s[3]; gv = (unsigned)strtoul(tmp, NULL, 16);
	tmp[0] = s[4]; tmp[1] = s[5]; bv = (unsigned)strtoul(tmp, NULL, 16);
	*r = rv / 255.0f;
	*g = gv / 255.0f;
	*b = bv / 255.0f;
	return 0;
}

/* separable box blur on float buffer (in-place using tmp) */
static void box_blur_separable(float *buf, float *tmp, int w, int h, int radius) {
	if (radius <= 0) return;

	int rw = radius * 2 + 1;

	/* horizontal pass -> tmp */
	for (int y = 0; y < h; y++) {
		int row = y * w;
		float sum = 0.0f;
		/* initial window */
		int x;
		for (x = 0; x <= radius && x < w; x++) sum += buf[row + x];
		for (x = radius + 1; x < rw && x < w; x++) sum += buf[row + x];
		for (x = 0; x < w; x++) {
			tmp[row + x] = sum / (float)rw;
			/* slide window */
			int add = x + radius + 1;
			int sub = x - radius;
			if (add < w) sum += buf[row + add];
			if (sub >= 0) sum -= buf[row + sub];
		}
	}

	/* vertical pass -> buf */
	for (int x = 0; x < w; x++) {
		float sum = 0.0f;
		int y;
		for (y = 0; y <= radius && y < h; y++) sum += tmp[y * w + x];
		for (y = radius + 1; y < rw && y < h; y++) sum += tmp[y * w + x];
		for (y = 0; y < h; y++) {
			buf[y * w + x] = sum / (float)rw;
			int add = y + radius + 1;
			int sub = y - radius;
			if (add < h) sum += tmp[add * w + x];
			if (sub >= 0) sum -= tmp[sub * w + x];
		}
	}
}

/* The actual filter */
void
filter_dropshadow(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data || !lock || !lock->m) return;

	/* read parameters with defaults */
	double opacity = (p->parameters[0] >= 0.0) ? CLAMP(p->parameters[0], 0.0f, 1.0f) : 0.5;
	double angle_deg = (p->parameters[1] != 0.0) ? p->parameters[1] : 45.0;
	double distance = (p->parameters[2] >= 0.0) ? p->parameters[2] : 20.0;
	double size = (p->parameters[3] >= 0.0) ? p->parameters[3] : 12.0;
	double noise = (p->parameters[4] >= 0.0) ? CLAMP(p->parameters[4], 0.0f, 1.0f) : 0.0;
	int mode = (int)p->parameters[5];
	int knockout = (p->parameters[6] != 0.0);

	/* tint color (default black) */
	float tint_r = 0.0f, tint_g = 0.0f, tint_b = 0.0f;
	if (p->num_string_parameters > 0 && p->string_parameters && p->string_parameters[0]) {
		if (parse_hex_color(p->string_parameters[0], &tint_r, &tint_g, &tint_b) != 0) {
			/* parse failed -> keep black */
			tint_r = tint_g = tint_b = 0.0f;
		}
	}


	int bpp = (img->bits_per_pixel + 7) / 8;
	int stride = img->bytes_per_line;

	/* seed RNG once */
	srand((unsigned)time(NULL));

	/* precompute offset */
	double ang = angle_deg * (M_PI / 180.0);
	int offx = (int)round(cos(ang) * distance);
	int offy = (int)round(-sin(ang) * distance); /* screen y increases downward, so negate */

	/* blur radius integer */
	int radius = (int)round(size);
	if (radius < 0) radius = 0;

	/* For each monitor allocate alpha buffers and build shadow */
	for (Monitor *m = lock->m; m; m = m->next) {
		int mw = m->mw;
		int mh = m->mh;
		if (mw <= 0 || mh <= 0) continue;

		size_t pixels = (size_t)mw * (size_t)mh;
		float *alpha = calloc(pixels, sizeof(float));
		float *tmp = calloc(pixels, sizeof(float));
		if (!alpha || !tmp) {
			free(alpha); free(tmp);
			return;
		}

		/* also keep source mask if knockout requested */
		unsigned char *src_mask = NULL;
		if (knockout) {
			src_mask = calloc(pixels, 1);
			if (!src_mask) { free(alpha); free(tmp); return; }
		}

		/* STEP 1: build the basic mask from source pixels (luminance) and scatter into offset positions */
		for (int y = 0; y < mh; y++) {
			unsigned char *row = (unsigned char *)img->data + (m->my + y) * stride;
			for (int x = 0; x < mw; x++) {
				unsigned char *px = row + (m->mx + x) * bpp;
				/* destination XImage is BGR(A/X) */
				float r = px[2] / 255.0f;
				float g = px[1] / 255.0f;
				float b = px[0] / 255.0f;
				/* mask: use luminance as proxy for opacity; very dark pixels produce small masks */
				float mask = 0.2126f * r + 0.7152f * g + 0.0722f * b;
				if (mask <= 0.001f) {
					/* nothing */
					continue;
				}
				/* mark source mask for knockout checks */
				if (knockout) src_mask[y * mw + x] = 1;

				/* place into offset location */
				int tx = x + offx;
				int ty = y + offy;
				if (tx < 0 || tx >= mw || ty < 0 || ty >= mh) continue;
				size_t idx = (size_t)ty * mw + tx;
				/* we want shadow alpha to represent presence/intensity; keep max to avoid weird accumulation */
				if (mask > alpha[idx]) alpha[idx] = mask;
			}
		}

		/* STEP 2: blur the alpha map with separable box blur (approx gaussian) */
		if (radius > 0) box_blur_separable(alpha, tmp, mw, mh, radius);

		/* STEP 3: apply noise (if requested) and clamp */
		if (noise > 0.0) {
			for (size_t i = 0; i < pixels; i++) {
				float jitter = 1.0f + (randf() - 0.5f) * 2.0f * (float)noise; /* in [1-noise,1+noise] */
				alpha[i] = CLAMP(alpha[i] * jitter, 0.0f, 1.0f);
			}
		}

		/* STEP 4: composite the shadow into the destination image (per-monitor coordinates) */
		for (int y = 0; y < mh; y++) {
			unsigned char *dst_row = (unsigned char *)img->data + (m->my + y) * stride;
			for (int x = 0; x < mw; x++) {
				size_t idx = (size_t)y * mw + x;
				float a = alpha[idx] * (float)opacity;
				if (a <= 0.0001f) continue;

				/* if knockout is set and original source present at same (un-offset) coord, skip drawing shadow */
				/* note: 'src_mask' marks original source pixels at original coords; we must check whether the
				   original pixel that maps here by offset exists. We used offx/offy with shadow source scatter:
				   a source at (sx,sy) contributed to alpha at (sx+offx, sy+offy). Now at target (x,y) the corresponding
				   source position is (x-offx, y-offy). If that source had a mask, we skip drawing when knockout true. */
				if (knockout) {
					int sx = x - offx;
					int sy = y - offy;
					if (sx >= 0 && sx < mw && sy >= 0 && sy < mh) {
						if (src_mask[sy * mw + sx]) continue;
					}
				}

				/* shadow tint color in 0..1 space */
				float sr = tint_r;
				float sg = tint_g;
				float sb = tint_b;

				/* final shadow alpha (byte) */
				uint8_t sa = (uint8_t)CLAMP(a * 255.0f, 0.0f, 255.0f);

				/* get pointer to destination pixel in XImage (BGRX) */
				unsigned char *dst_px = dst_row + (m->mx + x) * bpp;

				/* convert tint to 8-bit and call blend_pixel */
				uint8_t tr = (uint8_t)CLAMP(sr * 255.0f, 0.0f, 255.0f);
				uint8_t tg = (uint8_t)CLAMP(sg * 255.0f, 0.0f, 255.0f);
				uint8_t tb = (uint8_t)CLAMP(sb * 255.0f, 0.0f, 255.0f);

				/* composite using your blend_pixel (blend parameter set to 1.0 as alpha encoded in sa) */
				blend_pixel(dst_px, tr, tg, tb, sa, 1.0f, mode);
			}
		}

		free(alpha);
		free(tmp);
		free(src_mask);
	}
}
