void
filter_posterize_bit_depth(XImage *img, EffectParams *p, struct lock *lock)
{
	int bits = (int)p->parameters[0];
	int affect_r = (int)p->parameters[1];
	int affect_g = (int)p->parameters[2];
	int affect_b = (int)p->parameters[3];

	/* If no channels are specified then apply to all */
	if (!affect_r && !affect_g && !affect_b) {
		affect_r = affect_g = affect_b = 1;
	}

	if (!img || !img->data) return;
	if (bits < 1) bits = 1;
	if (bits > 16) bits = 16;

	Monitor *m;
	int bpp = img->bits_per_pixel / 8;
	if (bpp < 3) return; /* must have RGB */

	int stride = img->bytes_per_line;

	/* Posterization mask: keep top `bits` bits */
	int shift = 8 - bits;            /* how many LSBs to drop */
	uint8_t mask = 0xFF << shift;    /* e.g. bits=4 → 0xF0 */

	/* Iterate pixels */
	for (m = lock->m; m; m = m->next) {
		for (int y = m->my; y < m->my + m->mh; y++) {
			uint8_t *row = (uint8_t*)img->data + y * stride;
			for (int x = m->mx; x < m->mx + m->mw; x++) {
				uint8_t *px = row + x * bpp;

				/* BGRX order (XImage is often little-endian) */
				if (affect_b) {
					uint8_t b = px[0] & mask;
					px[0] = b | (b >> bits);  /* replicate bits downward */
				}
				if (affect_g) {
					uint8_t g = px[1] & mask;
					px[1] = g | (g >> bits);
				}
				if (affect_r) {
					uint8_t r = px[2] & mask;
					px[2] = r | (r >> bits);
				}
			}
		}
	}
}

void
filter_posterize(XImage *img, EffectParams *p, struct lock *lock)
{
	int levels = (int)p->parameters[0];
	int affect_r = (int)p->parameters[1];
	int affect_g = (int)p->parameters[2];
	int affect_b = (int)p->parameters[3];

	/* If no channels are specified then apply to all */
	if (!affect_r && !affect_g && !affect_b) {
		affect_r = affect_g = affect_b = 1;
	}

	if (levels < 2) levels = 2;

	Monitor *m;
	int bpp = img->bits_per_pixel / 8;
	int stride = img->bytes_per_line;

	for (m = lock->m; m; m = m->next) {
		for (int y = m->my; y < m->my + m->mh; y++) {
			uint8_t *row = (uint8_t*)img->data + y * stride;
			for (int x = m->mx; x < m->mx + m->mw; x++) {
				unsigned char *px = row + x * bpp;

				unsigned char r = px[2];
				unsigned char g = px[1];
				unsigned char b = px[0];

				/* Photoshop-like quantization */
				double scale = 255.0 / (levels - 1);

				if (affect_r) {
					r = (unsigned char)(round((r / 255.0) * (levels - 1)) * scale);
				}

				if (affect_g) {
					g = (unsigned char)(round((g / 255.0) * (levels - 1)) * scale);
				}

				if (affect_b) {
					b = (unsigned char)(round((b / 255.0) * (levels - 1)) * scale);
				}


				px[2] = r;
				px[1] = g;
				px[0] = b;
			}
		}
	}
}
