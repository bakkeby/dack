/*  Mosaic filter – creates a pixelated “tile” effect.
 *
 *  The first two parameters control the tile size:
 *      parameters[0] – horizontal tile width  (default 8)
 *      parameters[1] – vertical   tile height (default 8)
 *
 *  The remaining slots are unused but kept so the function matches the
 *  required 8‑parameter array layout.
 *
 *  Usage example:
 *      double p[8] = { 12, 12, 0 };
 *      filter_mosaic(img, p);
 */

void filter_mosaic(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data) return;

	/* -------------------------------------------------------------
	 *  Parameter handling – fall back to sensible defaults if the
	 *  caller supplies non‑positive values.
	 * ------------------------------------------------------------- */
	int tile_w = (int)(p->parameters[0] > 0 ? p->parameters[0] : 8);
	int tile_h = (int)(p->parameters[1] > 0 ? p->parameters[1] : 8);
	int red_mask_shift = 1;
	int green_mask_shift = 1;
	int blue_mask_shift = 1;
	int alpha_mask_shift = 1;

	/* Clamp tile dimensions to at least 1 pixel and at most the image size */
	if (tile_w < 1)   tile_w = 1;
	if (tile_h < 1)   tile_h = 1;
	if (tile_w > img->width)  tile_w = img->width;
	if (tile_h > img->height) tile_h = img->height;

	/* -------------------------------------------------------------
	 *  Core algorithm – for each tile we compute the average colour
	 *  of all pixels inside the tile and then write that colour back
	 *  to every pixel belonging to the same tile.
	 * ------------------------------------------------------------- */
	for (int y = 0; y < img->height; y += tile_h) {
		for (int x = 0; x < img->width; x += tile_w) {

			/* Accumulators for the RGBA components */
			unsigned long sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
			int count = 0;

			/* First pass – gather colour statistics for the tile */
			for (int ty = 0; ty < tile_h && (y + ty) < img->height; ++ty) {
				for (int tx = 0; tx < tile_w && (x + tx) < img->width; ++tx) {
					unsigned long pixel = XGetPixel(img,
						x + tx,
						y + ty);

					/* Extract components – assumes 24/32‑bit TrueColor */
					unsigned char r = (pixel >> red_mask_shift)   & 0xFF;
					unsigned char g = (pixel >> green_mask_shift) & 0xFF;
					unsigned char b = (pixel >> blue_mask_shift)  & 0xFF;
					unsigned char a = (img->depth == 32) ?
						(pixel >> alpha_mask_shift) & 0xFF : 0xFF;

					sum_r += r;
					sum_g += g;
					sum_b += b;
					sum_a += a;
					++count;
				}
			}

			/* Compute average colour for the tile */
			unsigned long avg_r = sum_r / count;
			unsigned long avg_g = sum_g / count;
			unsigned long avg_b = sum_b / count;

			/* Re‑assemble a pixel value in the native format */
			unsigned long avg_pixel =
				((avg_r << red_mask_shift)   & img->red_mask) |
				((avg_g << green_mask_shift) & img->green_mask) |
				((avg_b << blue_mask_shift)  & img->blue_mask);

			/* Second pass – write the averaged colour back to every pixel */
			for (int ty = 0; ty < tile_h && (y + ty) < img->height; ++ty) {
				for (int tx = 0; tx < tile_w && (x + tx) < img->width; ++tx) {
					XPutPixel(img,
						x + tx,
						y + ty,
						avg_pixel);
				}
			}
		}
	}
}
