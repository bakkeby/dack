void
filter_random_blocks(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data)
		return;

	int bpp    = img->bits_per_pixel / 8;
	int stride = img->bytes_per_line;

	/* Parameters:
	 * 0: block width (default: img->width / 16)
	 * 1: block height (default: img->height / 16)
	 */

	unsigned long pixel = lock->colors[BLOCKS];
	int y_count = p->parameters[3] > 0 ? (int)p->parameters[3] : 20;
	int x_count = p->parameters[2] > 0 ? (int)p->parameters[2] : y_count * img->width/img->height;
	int min_x = (int)p->parameters[4];
	int max_x = (int)p->parameters[5];
	int min_y = (int)p->parameters[6];
	int max_y = (int)p->parameters[7];

	/* Fall back to showing on the primary screen only */
	if (!min_y && !max_y && !min_x && !max_x) {
		min_y = lock->m->my;
		min_x = lock->m->mx;
		max_y = min_y + lock->m->mh;
		max_x = min_x + lock->m->mw;
	}

	int area_width = img->width - min_x;
	int area_height = img->height - min_y;

	if (max_x && area_width > max_x - min_x) {
		area_width = max_x - min_x;
	}

	if (max_y && area_height > max_y - min_y) {
		area_height = max_y - min_y;
	}

	int block_w = p->parameters[0] > 0 ? (int)p->parameters[0] : area_width / x_count;
	int block_h = p->parameters[1] > 0 ? (int)p->parameters[1] : area_height / y_count;

	/* Pick a random position */
	int x = min_x + rand() % (area_width  - block_w);
	int y = min_y + rand() % (area_height - block_h);

	/* Draw the rectangle directly into the buffer */
	for (int yy = 0; yy < block_h; yy++) {
		if (y + yy >= img->height) break;
		uint8_t *row = (uint8_t *)img->data + (y + yy) * stride + x * bpp;
		for (int xx = 0; xx < block_w; xx++) {
			if (x + xx >= img->width) break;
			uint8_t *px = row + xx * bpp;
			/* Write the color bytes in native endian */
			for (int i = 0; i < bpp && i < 4; i++) {
				px[i] = (pixel >> (8 * i)) & 0xFF;
			}
		}
	}
}
