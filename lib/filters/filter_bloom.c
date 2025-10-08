void
filter_bloom(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data) return;

	int bpp = (img->bits_per_pixel + 7) / 8;
	if (bpp < 3) return;
	int stride = img->bytes_per_line;

	/* Parameters:
	 *    0 = threshold (0–255, default 100)
	 *    1 = blur radius (default 5)
	 *    2 = intensity (blend factor, 0–1, default 0.9)
	 */
	int threshold = (p->parameters[0] > 0 ? (int)p->parameters[0] : 100);
	int radius = (p->parameters[1] > 0 ? (int)p->parameters[1] : 5);
	double intensity = (p->parameters[2] > 0 ? p->parameters[2] : 0.9);
	if (intensity > 1.0) intensity = 1.0;

	/* Allocate buffer for bright-pass image */
	uint8_t *bright = calloc(1, stride * img->height);
	if (!bright)
		return;

	/* Step 1: Extract bright areas into greyscale mask */
	for (int y = 0; y < img->height; y++) {
		uint8_t *row = (uint8_t*)img->data + y * stride;
		uint8_t *maskrow = bright + y * stride;
		for (int x = 0; x < img->width; x++) {
			uint8_t *px = row + x * bpp;
			int lum = (px[0] + px[1] + px[2]) / 3;
			if (lum > threshold) {
				maskrow[x * bpp + 0] = px[0];
				maskrow[x * bpp + 1] = px[1];
				maskrow[x * bpp + 2] = px[2];
			}
		}
	}

	/* Step 2: Blur the bright areas (simple box blur) */
	uint8_t *blurred = calloc(1, stride * img->height);
	if (!blurred) {
		free(bright);
		return;
	}

	for (int y = 0; y < img->height; y++) {
		for (int x = 0; x < img->width; x++) {
			int sum[3] = {0,0,0};
			int count = 0;

			for (int dy = -radius; dy <= radius; dy++) {
				int yy = y + dy;
				if (yy < 0 || yy >= img->height) continue;
				uint8_t *row = bright + yy * stride;
				for (int dx = -radius; dx <= radius; dx++) {
					int xx = x + dx;
					if (xx < 0 || xx >= img->width) continue;
					uint8_t *px = row + xx * bpp;
					sum[0] += px[0];
					sum[1] += px[1];
					sum[2] += px[2];
					count++;
				}
			}

			uint8_t *dst = blurred + y * stride + x * bpp;
			dst[0] = sum[0] / count;
			dst[1] = sum[1] / count;
			dst[2] = sum[2] / count;
		}
	}

	/* Step 3: Blend blurred highlights back into the image */
	for (int y = 0; y < img->height; y++) {
		uint8_t *orig = (uint8_t*)img->data + y * stride;
		uint8_t *glow = blurred + y * stride;
		for (int x = 0; x < img->width; x++) {
			uint8_t *o = orig + x * bpp;
			uint8_t *g = glow + x * bpp;
			for (int c = 0; c < 3; c++) {
				int val = o[c] + (int)(g[c] * intensity);
				if (val > 255) val = 255;
				o[c] = (uint8_t)val;
			}
		}
	}

	free(bright);
	free(blurred);
}
