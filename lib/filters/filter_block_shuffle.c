#define PIXEL_PTR(img, x, y) \
	((unsigned char *)(img)->data + (y) * (img)->bytes_per_line + \
	 (x) * ((img)->bits_per_pixel + 7) / 8)

/* ----------------------------------------------------------------------
 * filter_block_shuffle
 * ----------------------------------------------------------------------
 * parameters[0] = tile width   (default 32)
 * parameters[1] = tile height  (default 32)
 * parameters[2] = randomize?   (0 = reverse order, 1 = random shuffle)
 * parameters[3] = scramble probability (0.0–1.0, default=1.0)
 * ---------------------------------------------------------------------- */
void
filter_block_shuffle(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data || !lock) return;

	int bpp    = (img->bits_per_pixel + 7) / 8;
	int stride = img->bytes_per_line;

	int tile_w = (int)(p->parameters[0] > 0 ? p->parameters[0] : 32);
	int tile_h = (int)(p->parameters[1] > 0 ? p->parameters[1] : 32);
	int do_rand = (int)(p->parameters[2] > 0 ? 1 : 0);
	double prob = (p->parameters[3] >= 0 && p->parameters[3] <= 1) ? p->parameters[3] : 1.0;

	if (tile_w < 1) tile_w = 1;
	if (tile_h < 1) tile_h = 1;

	/* Copy the original image data so we don't overwrite tiles before reading them */
	unsigned char *src_copy = malloc(stride * img->height);
	if (!src_copy) return;
	memcpy(src_copy, img->data, stride * img->height);

	for (Monitor *m = lock->m; m; m = m->next) {
		int tiles_x = m->mw / tile_w;
		int tiles_y = m->mh / tile_h;
		int num_tiles = tiles_x * tiles_y;

		if (num_tiles <= 1) continue;

		/* Build tile index array */
		int *indices = malloc(num_tiles * sizeof(int));
		if (!indices) continue;

		for (int i = 0; i < num_tiles; i++)
			indices[i] = i;

		/* Shuffle or reverse */
		if (do_rand) {
			for (int i = num_tiles - 1; i > 0; i--) {
				int j = rand() % (i + 1);
				int tmp = indices[i];
				indices[i] = indices[j];
				indices[j] = tmp;
			}
		} else {
			for (int i = 0; i < num_tiles; i++)
				indices[i] = num_tiles - 1 - i;
		}

		/* Apply shuffle: read from src_copy → write into img->data */
		for (int t = 0; t < num_tiles; t++) {
			/* Apply probability: skip some tiles */
			if (((double)rand() / RAND_MAX) > prob) {
				continue;
			}

			int src_idx = t;
			int dst_idx = indices[t];

			int sx = m->mx + (src_idx % tiles_x) * tile_w;
			int sy = m->my + (src_idx / tiles_x) * tile_h;
			int dx = m->mx + (dst_idx % tiles_x) * tile_w;
			int dy = m->my + (dst_idx / tiles_x) * tile_h;

			/* Clamp for edge tiles */
			int cur_w = (sx + tile_w > m->mx + m->mw) ? (m->mx + m->mw - sx) : tile_w;
			int cur_h = (sy + tile_h > m->my + m->mh) ? (m->my + m->mh - sy) : tile_h;

			for (int row = 0; row < cur_h; row++) {
				unsigned char *src = src_copy + (sy + row) * stride + sx * bpp;
				unsigned char *dst = (unsigned char *)img->data + (dy + row) * stride + dx * bpp;
				memcpy(dst, src, cur_w * bpp);
			}
		}

		free(indices);
	}

	free(src_copy);
}

#undef PIXEL_PTR
