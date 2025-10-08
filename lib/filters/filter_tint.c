/* ----------------------------------------------------------------------
 * filter_tint
 * parameters[0] = red   (0–255)
 * parameters[1] = green (0–255)
 * parameters[2] = blue  (0–255)
 * parameters[3] = blend strength (0.0–1.0)
 * ---------------------------------------------------------------------- */
void
filter_tint(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data) return;

	int tint_r = (int)p->parameters[0];
	int tint_g = (int)p->parameters[1];
	int tint_b = (int)p->parameters[2];
	double blend = (p->parameters[3] >= 0.0 && p->parameters[3] <= 1.0) ? p->parameters[3] : 0.5;

	int bpp    = (img->bits_per_pixel + 7) / 8;
	int stride = img->bytes_per_line;

	for (int y = 0; y < img->height; y++) {
		unsigned char *row = (unsigned char *)img->data + y * stride;
		for (int x = 0; x < img->width; x++) {
			unsigned char *px = row + x * bpp;

			int b = px[0];
			int g = px[1];
			int r = px[2];

			/* blend original with tint */
			int nr = (int)((1.0 - blend) * r + blend * tint_r);
			int ng = (int)((1.0 - blend) * g + blend * tint_g);
			int nb = (int)((1.0 - blend) * b + blend * tint_b);

			if (nr < 0) { nr = 0; }; if (nr > 255) { nr = 255; };
			if (ng < 0) { ng = 0; }; if (ng > 255) { ng = 255; };
			if (nb < 0) { nb = 0; }; if (nb > 255) { nb = 255; };

			px[2] = (unsigned char)nr;
			px[1] = (unsigned char)ng;
			px[0] = (unsigned char)nb;
		}
	}
}
