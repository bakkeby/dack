#include <math.h>

typedef struct {
	float x, y;
} Point;

static inline float frand(unsigned int *seed) {
	*seed = 1664525 * (*seed) + 1013904223;
	return ((*seed >> 16) & 0x7FFF) / 32767.0f;
}

void
filter_voronoi_diagram(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !lock || !lock->m) return;

	// Parameters
	double crack_intensity = p->parameters[0]; // e.g. 1.5–3.0
	double cell_size       = p->parameters[1]; // e.g. 64–128 pixels
	double blend           = p->parameters[2]; // 0.0–1.0
	int num_points         = (int)p->parameters[3]; // number of Voronoi seed points, e.g. 64

	if (cell_size < 4.0) cell_size = 4.0;
	if (blend < 0.0) blend = 0.0;
	if (blend > 1.0) blend = 1.0;
	if (num_points <= 0) num_points = 64;

	int stride = img->bytes_per_line;
	int bpp = img->bits_per_pixel / 8;

	// Loop over monitors
	for (Monitor *mon = lock->m; mon; mon = mon->next) {
		// Generate Voronoi seed points for this monitor
		Point points[num_points];
		unsigned int seed = mon->mx + mon->my + mon->mw * mon->mh;

		for (int i = 0; i < num_points; i++) {
			points[i].x = mon->mx + frand(&seed) * mon->mw;
			points[i].y = mon->my + frand(&seed) * mon->mh;
		}

		for (int y = mon->my; y < mon->my + mon->mh; y++) {
			for (int x = mon->mx; x < mon->mx + mon->mw; x++) {
				unsigned char *px = (unsigned char *)img->data + y * stride + x * bpp;

				// Find nearest two points
				float d1 = 1e9f, d2 = 1e9f;
				for (int i = 0; i < num_points; i++) {
					float dx = x - points[i].x;
					float dy = y - points[i].y;
					float dist = dx * dx + dy * dy;
					if (dist < d1) {
						d2 = d1;
						d1 = dist;
					} else if (dist < d2) {
						d2 = dist;
					}
				}

				// Distance to nearest edge between cells
				float crack = fabsf(sqrtf(d1) - sqrtf(d2)) / cell_size;

				// Sharpen with intensity
				float factor = expf(-crack * crack_intensity * 10.0f) * blend;

				px[0] = (unsigned char)(px[0] * (1.0f - factor));
				px[1] = (unsigned char)(px[1] * (1.0f - factor));
				px[2] = (unsigned char)(px[2] * (1.0f - factor));
			}
		}
	}
}

static inline double value_noise(int x, int y, int seed) {
	int n = x + y * 57 + seed * 131;
	n = (n << 13) ^ n;
	return (1.0 - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}

static double fbm_noise(double x, double y, int octaves, double persistence, int seed) {
	double total = 0, freq = 1, amp = 1, max = 0;
	for (int i = 0; i < octaves; i++) {
		total += value_noise((int)(x * freq), (int)(y * freq), seed) * amp;
		max += amp;
		amp *= persistence;
		freq *= 2.0;
	}
	return total / max;
}

void
filter_reticulation(XImage *img, EffectParams *p, struct lock *lock)
{
	/* Parameters */
	double scale = p->parameters[0] > 0 ? p->parameters[0] : 8.0;	/* noise scale */
	double threshold = p->parameters[1];			/* crack threshold (0..1) */
	double blend = (p->parameters[2] >= 0 && p->parameters[2] <= 1) ? p->parameters[2] : 0.5;
	int octaves = p->parameters[3] > 0 ? (int)p->parameters[3] : 3;
	int seed = (int)p->parameters[4];

	int bpp = (img->bits_per_pixel + 7) / 8;
	int stride = img->bytes_per_line;

	for (Monitor *m = lock->m; m; m = m->next) {
		for (int y = 0; y < m->mh; y++) {
			unsigned char *row = (unsigned char *)img->data + (m->my + y) * stride + m->mx * bpp;
			for (int x = 0; x < m->mw; x++) {
				double nx = (double)x / scale;
				double ny = (double)y / scale;
				double n = fbm_noise(nx, ny, octaves, 0.5, seed);
				n = fabs(n);	/* more vein-like */
				double mask = (n > threshold) ? 1.0 : 0.0;

				unsigned char *p = row + x * bpp;

				/* darken where mask is 1 */
				for (int c = 0; c < 3; c++) {
					int val = p[c];
					val = (int)(val * (1.0 - mask * blend));
					if (val < 0) val = 0;
					p[c] = (unsigned char)val;
				}
			}
		}
	}
}
