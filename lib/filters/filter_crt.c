void
filter_crt_effect(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data) return;

	/* Parameters */
	double scanline_intensity = (p->parameters[0] > 0) ? p->parameters[0] : 0.3; /* 0.0–1.0 */
	double flicker_strength   = (p->parameters[1] > 0) ? p->parameters[1] : 0.05; /* 0.0–0.2 */
	int phosphor_stripes      = (p->parameters[2] > 0.5); /* 0 = off, 1 = on */
	int invert_stripes        = (p->parameters[3] > 0.5); /* shift stripe order */
	double glow_intensity     = (p->parameters[4] > 0) ? p->parameters[4] : 0.25; /* 0.0–1.0 */

	int bpp = img->bits_per_pixel / 8;
	if (bpp < 3) return;

	int stride = img->bytes_per_line;

	/* Random flicker */
	double flicker = 1.0 + ((rand() % 200 - 100) / 100.0) * flicker_strength;

	/* Backup original image for bloom pass */
	size_t bufsize = stride * img->height;
	uint8_t *orig = malloc(bufsize);
	if (!orig) return;
	memcpy(orig, img->data, bufsize);

	for (int y = 0; y < img->height; y++) {
		uint8_t *row = (uint8_t*)img->data + y * stride;

		for (int x = 0; x < img->width; x++) {
			uint8_t *dst = row + x * bpp;
			uint8_t *src = orig + y * stride + x * bpp;

			/* Start from original pixel */
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			if (bpp == 4) dst[3] = 0xff;

			/* --- Apply scanlines --- */
			if (y % 2 == 1) {
				dst[0] = (uint8_t)(dst[0] * (1.0 - scanline_intensity));
				dst[1] = (uint8_t)(dst[1] * (1.0 - scanline_intensity));
				dst[2] = (uint8_t)(dst[2] * (1.0 - scanline_intensity));
			}

			/* --- Apply phosphor stripes --- */
			if (phosphor_stripes) {
				int stripe = (x + (invert_stripes ? 1 : 0)) % 3;
				if (stripe == 0) { /* red stripe */
					dst[1] /= 3; dst[0] /= 3;
				} else if (stripe == 1) { /* green stripe */
					dst[2] /= 3; dst[0] /= 3;
				} else { /* blue stripe */
					dst[2] /= 3; dst[1] /= 3;
				}
			}

			/* --- Apply flicker --- */
			dst[0] = (uint8_t)fmin(255, dst[0] * flicker);
			dst[1] = (uint8_t)fmin(255, dst[1] * flicker);
			dst[2] = (uint8_t)fmin(255, dst[2] * flicker);
		}
	}

	/* --- Bloom/Glow pass --- */
	if (glow_intensity > 0.0) {
		int radius = 2; /* small neighborhood for glow */
		for (int y = 0; y < img->height; y++) {
			uint8_t *row = (uint8_t*)img->data + y * stride;
			for (int x = 0; x < img->width; x++) {
				uint8_t *dst = row + x * bpp;

				int rsum = 0, gsum = 0, bsum = 0, count = 0;

				/* Sample a small box around the pixel */
				for (int dy = -radius; dy <= radius; dy++) {
					int sy = y + dy;
					if (sy < 0 || sy >= img->height) continue;
					uint8_t *srow = (uint8_t*)orig + sy * stride;
					for (int dx = -radius; dx <= radius; dx++) {
						int sx = x + dx;
						if (sx < 0 || sx >= img->width) continue;
						uint8_t *spx = srow + sx * bpp;
						rsum += spx[2];
						gsum += spx[1];
						bsum += spx[0];
						count++;
					}
				}

				if (count > 0) {
					uint8_t glow_r = (rsum / count);
					uint8_t glow_g = (gsum / count);
					uint8_t glow_b = (bsum / count);

					/* Blend glow into pixel */
					dst[2] = (uint8_t)fmin(255, dst[2] + glow_intensity * glow_r);
					dst[1] = (uint8_t)fmin(255, dst[1] + glow_intensity * glow_g);
					dst[0] = (uint8_t)fmin(255, dst[0] + glow_intensity * glow_b);
				}
			}
		}
	}

	free(orig);
}
