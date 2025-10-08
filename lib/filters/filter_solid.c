/* Allocate backing buffer for an XImage and fill with a solid pixel value */
void
filter_solid_color(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img)
		return;

	int color_idx = CLAMP((int)p->parameters[0], 0, NUMCOLS - 1);
	unsigned long pixel = lock->colors[color_idx];

	/* Allocate data if needed */
	if (!img->data) {
		img->data = malloc(img->bytes_per_line * img->height);
		if (!img->data)
			return; /* OOM */
	}

	int bpp    = img->bits_per_pixel / 8;   /* bytes per pixel (rounded down) */
	int stride = img->bytes_per_line;
	int width  = img->width;
	int height = img->height;

	/* Build a single pixel in native endian */
	uint8_t pixbuf[4] = {0};
	for (int i = 0; i < bpp && i < 4; i++) {
		pixbuf[i] = (pixel >> (8 * i)) & 0xFF;
	}

	/* Build one scanline */
	uint8_t *scanline = malloc(width * bpp);
	if (!scanline)
		return;

	for (int x = 0; x < width; x++)
		memcpy(scanline + x * bpp, pixbuf, bpp);

	/* Copy scanline across entire image */
	for (int y = 0; y < height; y++) {
		memcpy((uint8_t *)img->data + y * stride, scanline, width * bpp);
	}

	free(scanline);
}
