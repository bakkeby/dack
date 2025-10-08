/* -------------------------------------------------------------
   vertical rain shift
   -------------------------------------------------------------
   Shift each column downward by a random offset in the range
   [0 .. max_shift].  Pixels that fall off the bottom wrap to the
   top.  The operation is performed in‑place on the XImage.
   ------------------------------------------------------------- */
void
filter_rain_shift_vert(XImage *img, EffectParams *p, struct lock *lock)
{
	int max_shift = (int)p->parameters[0];

	if (!img || max_shift <= 0)
		return;

	Monitor *m;
	int bpp = img->bits_per_pixel / 8;          /* bytes per pixel */
	if (bpp == 0) bpp = 4;                      /* safety fallback */

	unsigned char *col_buf = malloc(img->height * bpp);
	if (!col_buf)
		return;

	for (m = lock->m; m; m = m->next) {
		for (int x = m->mx; x < m->mx + m->mw; ++x) {
			/* copy column into temporary buffer */
			for (int y = m->my; y < m->my + m->mh; ++y) {
				unsigned char *src = (unsigned char *)img->data +
									 y * img->bytes_per_line + x * bpp;
				memcpy(col_buf + y * bpp, src, bpp);
			}

			int offset = rand() % (max_shift + 1);   /* 0 … max_shift */

			/* write column back, shifted downwards (wrap around) */
			for (int y = 0; y < m->mh; ++y) {
				int src_y = m->my + (y - offset + m->mh) % m->mh;
				unsigned char *dst = (unsigned char *)img->data +
									 (m->my + y) * img->bytes_per_line + x * bpp;
				memcpy(dst, col_buf + src_y * bpp, bpp);
			}
		}
	}
	free(col_buf);
}

/* -------------------------------------------------------------
   horzontal rain shift
   -------------------------------------------------------------
   Shift each row left/right by a random offset in the range
   [0 .. max_shift].  Positive offsets move the row to the right,
   negative offsets move it to the left.  Wrapping is performed so
   no pixels are lost.  The operation is performed in‑place on the
   XImage.
   ------------------------------------------------------------- */
void
filter_rain_shift_horz(XImage *img, EffectParams *p, struct lock *lock)
{
	int max_shift = (int)p->parameters[0];

	if (!img || max_shift <= 0)
		return;

	Monitor *m;
	int bpp = img->bits_per_pixel / 8;          /* bytes per pixel */
	if (bpp == 0) bpp = 4;                      /* safety fallback */

	/* Temporary buffer that holds one full row */
	unsigned char *row_buf = malloc(img->width * bpp);
	if (!row_buf) return;                       /* OOM guard */

	for (m = lock->m; m; m = m->next) {
		for (int y = m->my; y < m->my + m->mh; ++y) {
			/* copy the whole row into row_buf */
			unsigned char *src_row = (unsigned char *)img->data +
									 y * img->bytes_per_line;
			memcpy(row_buf, src_row, img->width * bpp);

			/* pick a random horizontal offset: 0 … max_shift */
			int offset = rand() % (max_shift + 1);

			/* optionally randomise direction (left/right) */
			if (rand() & 1) offset = -offset;      /* 50 % left, 50 % right */

			/* write the row back, shifted with wrap‑around */
			for (int x = 0; x <  m->mw; ++x) {
				int src_x = m->mx + (x - offset + m->mw) % m->mw;
				unsigned char *dst = src_row + (m->mx + x) * bpp;
				memcpy(dst, row_buf + src_x * bpp, bpp);
			}
		}
	}
	free(row_buf);
}
