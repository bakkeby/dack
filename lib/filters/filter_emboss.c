/* ----------------------------------------------------------------------
 * filter_emboss
 * parameters[0] = intensity (scaling of relief, default ~1.0)
 * parameters[1] = direction X offset (e.g. 1)
 * parameters[2] = direction Y offset (e.g. 1)
 * parameters[3] = blend strength (0.0–1.0, 1.0 replaces fully)
 * ---------------------------------------------------------------------- */
void
filter_emboss(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data) return;

	double intensity = (p->parameters[0] != 0.0) ? p->parameters[0] : 1.0;
	int dx = (p->parameters[1] != 0.0) ? (int)p->parameters[1] : 1;
	int dy = (p->parameters[2] != 0.0) ? (int)p->parameters[2] : 1;
	double blend = (p->parameters[3] >= 0.0 && p->parameters[3] <= 1.0) ? p->parameters[3] : 1.0;

	int bpp    = (img->bits_per_pixel + 7) / 8;
	int stride = img->bytes_per_line;

	/* Copy original image into buffer */
	size_t bufsize = (size_t)img->height * stride;
	unsigned char *src_copy = malloc(bufsize);
	if (!src_copy) return;
	memcpy(src_copy, img->data, bufsize);

	for (int y = 0; y < img->height; y++) {
		for (int x = 0; x < img->width; x++) {
			int nx = x + dx;
			int ny = y + dy;

			if (nx < 0 || ny < 0 || nx >= img->width || ny >= img->height)
				continue;

			unsigned char *p1 = src_copy + y * stride + x * bpp;
			unsigned char *p2 = src_copy + ny * stride + nx * bpp;
			unsigned char *dst = (unsigned char *)img->data + y * stride + x * bpp;

			int r1 = p1[2], g1 = p1[1], b1 = p1[0];
			int r2 = p2[2], g2 = p2[1], b2 = p2[0];

			int dr = CLAMP255((intensity * (r1 - r2)) + 128);
			int dg = CLAMP255((intensity * (g1 - g2)) + 128);
			int db = CLAMP255((intensity * (b1 - b2)) + 128);

			/* blend */
			dst[2] = (unsigned char)((1.0 - blend) * r1 + blend * dr);
			dst[1] = (unsigned char)((1.0 - blend) * g1 + blend * dg);
			dst[0] = (unsigned char)((1.0 - blend) * b1 + blend * db);
		}
	}

	free(src_copy);
}