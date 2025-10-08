void
filter_pixelate(XImage *img, EffectParams *p, struct lock *lock)
{
	int block_size = (int)p->parameters[0];
	int width  = img->width;
	int height = img->height;
	int bpp    = img->bits_per_pixel / 8;
	int pitch  = img->bytes_per_line;

	for (int by = 0; by < height; by += block_size) {
		for (int bx = 0; bx < width; bx += block_size) {
			int rsum = 0, gsum = 0, bsum = 0, count = 0;

			// 1. Compute block average
			for (int y = by; y < by + block_size && y < height; y++) {
				uint8_t *row = (uint8_t *)img->data + y * pitch;
				for (int x = bx; x < bx + block_size && x < width; x++) {
					uint8_t *px = row + x * bpp;
					bsum += px[0];
					gsum += px[1];
					rsum += px[2];
					count++;
				}
			}

			if (count == 0) continue;
			uint8_t r = rsum / count;
			uint8_t g = gsum / count;
			uint8_t b = bsum / count;

			// 2. Fill block with average color
			for (int y = by; y < by + block_size && y < height; y++) {
				uint8_t *row = (uint8_t *)img->data + y * pitch;
				for (int x = bx; x < bx + block_size && x < width; x++) {
					uint8_t *px = row + x * bpp;
					px[0] = b;
					px[1] = g;
					px[2] = r;
				}
			}
		}
	}
}
