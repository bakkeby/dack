void
filter_sobel(XImage *img, EffectParams *p, struct lock *lock)
{
	int width  = img->width;
	int height = img->height;
	int bpp    = img->bits_per_pixel / 8; // 4
	int stride = img->bytes_per_line;

	uint8_t *data = (uint8_t *)img->data;
	uint8_t *tmp = malloc(stride * height);
	memcpy(tmp, data, stride * height);  // preserve original

	int Gx[3][3] = {
		{-1, 0, 1},
		{-2, 0, 2},
		{-1, 0, 1}
	};

	int Gy[3][3] = {
		{-1, -2, -1},
		{ 0,  0,  0},
		{ 1,  2,  1}
	};

	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			int sumRx = 0, sumGx = 0, sumBx = 0;
			int sumRy = 0, sumGy = 0, sumBy = 0;

			// Apply 3x3 Sobel kernel
			for (int ky = -1; ky <= 1; ky++) {
				uint8_t *row = tmp + (y + ky) * stride;
				for (int kx = -1; kx <= 1; kx++) {
					uint8_t *p = row + (x + kx) * bpp;
					int kxval = Gx[ky + 1][kx + 1];
					int kyval = Gy[ky + 1][kx + 1];

					sumBx += p[0] * kxval;
					sumGx += p[1] * kxval;
					sumRx += p[2] * kxval;

					sumBy += p[0] * kyval;
					sumGy += p[1] * kyval;
					sumRy += p[2] * kyval;
				}
			}

			uint8_t *dst = data + y * stride + x * bpp;
			dst[0] = clamp255((int)sqrt(sumBx * sumBx + sumBy * sumBy));
			dst[1] = clamp255((int)sqrt(sumGx * sumGx + sumGy * sumGy));
			dst[2] = clamp255((int)sqrt(sumRx * sumRx + sumRy * sumRy));
			// keep dst[3] as is
		}
	}

	free(tmp);
}