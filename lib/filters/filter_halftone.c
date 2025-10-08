/* ----------------------------------------------------------------------
 * filter_halftone
 * Multi-pattern halftone with blend and jitter
 * parameters[0] = period (cell size, default 8)
 * parameters[1] = angle (degrees, default 0)
 * parameters[2] = invert (0=black dots, 1=white dots)
 * parameters[3] = blend (0.0–1.0)
 * parameters[4] = jitter (0.0–1.0)
 * parameters[5] = pattern (0=circle, 1=line, 2=diamond)
 * ---------------------------------------------------------------------- */
void
filter_halftone(XImage *img, EffectParams *p, struct lock *lock)
{
	int period   = (p->parameters[0] > 0) ? (int)p->parameters[0] : 8;
	double angle = p->parameters[1];
	int invert   = (p->parameters[2] > 0.5) ? 1 : 0;
	double blend = p->parameters[3];
	if (blend < 0.0) blend = 0.0;
	if (blend > 1.0) blend = 1.0;
	double jitter = p->parameters[4];
	if (jitter < 0.0) jitter = 0.0;
	if (jitter > 1.0) jitter = 1.0;
	int pattern = (p->parameters[5] >= 0 && p->parameters[5] <= 2) ? (int)p->parameters[5] : 0;

	double radians = angle * M_PI / 180.0;
	double cos_a = cos(radians);
	double sin_a = sin(radians);

	Monitor *m;
	int bpp = img->bits_per_pixel / 8;
	int stride = img->bytes_per_line;

	for (m = lock->m; m; m = m->next) {
		for (int y = m->my; y < m->my + m->mh; y++) {
			unsigned char *row = (unsigned char*)img->data + y * stride;
			for (int x = m->mx; x < m->mx + m->mw; x++) {
				unsigned char *px = row + x * bpp;

				unsigned char r0 = px[2], g0 = px[1], b0 = px[0];
				double lum = 0.299*r0 + 0.587*g0 + 0.114*b0;

				double xr = x * cos_a + y * sin_a;
				double yr = -x * sin_a + y * cos_a;

				int cell_x = (int)floor(xr / period);
				int cell_y = (int)floor(yr / period);

				/* jittered cell center */
				double cx = (cell_x + 0.5) * period + (rand()/(double)RAND_MAX - 0.5) * jitter * period;
				double cy = (cell_y + 0.5) * period + (rand()/(double)RAND_MAX - 0.5) * jitter * period;

				double dx = xr - cx;
				double dy = yr - cy;
				double dist2 = dx*dx + dy*dy;

				double radius = (lum / 255.0) * (period / 2.0);
				int inside = 0;

				switch (pattern) {
					case 0: /* circle */
						inside = (dist2 < radius*radius);
						break;
					case 1: /* line */
						inside = (fabs(dy) < radius*0.5); /* horizontal line within radius */
						break;
					case 2: /* diamond */
						inside = (fabs(dx) + fabs(dy) < radius);
						break;
				}

				unsigned char dot = invert ? 255 : 0;
				unsigned char bg  = invert ? 0 : 255;
				unsigned char r1 = inside ? dot : bg;
				unsigned char g1 = inside ? dot : bg;
				unsigned char b1 = inside ? dot : bg;

				px[2] = (unsigned char)(r0*(1.0-blend) + r1*blend);
				px[1] = (unsigned char)(g0*(1.0-blend) + g1*blend);
				px[0] = (unsigned char)(b0*(1.0-blend) + b1*blend);
			}
		}
	}
}
