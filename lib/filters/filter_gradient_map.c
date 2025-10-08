static int hex_to_rgb(const char *hex, unsigned char *r, unsigned char *g, unsigned char *b)
{
	if (!hex || hex[0] != '#' || strlen(hex) != 7)
		return -1;
	char rs[3] = { hex[1], hex[2], '\0' };
	char gs[3] = { hex[3], hex[4], '\0' };
	char bs[3] = { hex[5], hex[6], '\0' };
	*r = (unsigned char)strtol(rs, NULL, 16);
	*g = (unsigned char)strtol(gs, NULL, 16);
	*b = (unsigned char)strtol(bs, NULL, 16);
	return 0;
}

struct GradientStop {
	double pos;
	unsigned char r, g, b;
};

struct Gradient {
	int count;
	struct GradientStop *stops;
};

/* Linear interpolate between two stops */
static void gradient_lerp(struct GradientStop *a, struct GradientStop *b, double t,
                          unsigned char *r, unsigned char *g, unsigned char *bcol) {
	*r = (unsigned char)((1.0 - t) * a->r + t * b->r);
	*g = (unsigned char)((1.0 - t) * a->g + t * b->g);
	*bcol = (unsigned char)((1.0 - t) * a->b + t * b->b);
}

static void gradient_lookup(struct Gradient *grad, double pos,
                            unsigned char *r, unsigned char *g, unsigned char *b) {
	if (pos <= grad->stops[0].pos) {
		*r = grad->stops[0].r;
		*g = grad->stops[0].g;
		*b = grad->stops[0].b;
		return;
	}
	if (pos >= grad->stops[grad->count - 1].pos) {
		*r = grad->stops[grad->count - 1].r;
		*g = grad->stops[grad->count - 1].g;
		*b = grad->stops[grad->count - 1].b;
		return;
	}
	for (int i = 0; i < grad->count - 1; i++) {
		struct GradientStop *ag = &grad->stops[i];
		struct GradientStop *bg = &grad->stops[i + 1];
		if (pos >= ag->pos && pos <= bg->pos) {
			double t = (pos - ag->pos) / (bg->pos - ag->pos);
			gradient_lerp(ag, bg, t, r, g, b);
			return;
		}
	}
}

/* Parse "0:#000000,0.5:#FF0000,1:#FFFFFF" */
static struct Gradient *parse_gradient_string(const char *str)
{
	if (!str) return NULL;

	/* Work on a copy so we can strtok safely */
	char *buf = strdup(str);
	if (!buf) return NULL;

	int capacity = 8, count = 0;
	struct GradientStop *stops = malloc(capacity * sizeof(*stops));
	if (!stops) { free(buf); return NULL; }

	char *token = strtok(buf, ",");
	while (token) {
		double pos = 0.0;
		char color[16];
		if (sscanf(token, "%lf:%15s", &pos, color) == 2) {
			if (count >= capacity) {
				capacity *= 2;
				stops = realloc(stops, capacity * sizeof(*stops));
				if (!stops) break;
			}
			unsigned char r,g,b;
			if (hex_to_rgb(color, &r, &g, &b) == 0) {
				stops[count].pos = pos;
				stops[count].r = r;
				stops[count].g = g;
				stops[count].b = b;
				count++;
			}
		}
		token = strtok(NULL, ",");
	}
	free(buf);

	if (count == 0) {
		free(stops);
		return NULL;
	}

	struct Gradient *grad = malloc(sizeof(*grad));
	grad->count = count;
	grad->stops = stops;
	return grad;
}

void
filter_gradient_map(XImage *img, EffectParams *p, struct lock *lock)
{
	if (!img || !img->data || !lock || !lock->m) return;

	/* Expect first string parameter = gradient definition */
	if (p->num_string_parameters == 0) return;
	struct Gradient *grad = parse_gradient_string(p->string_parameters[0]);
	if (!grad) return;

	double blend = (p->parameters[0] && p->parameters[0] <= 1.0) ? p->parameters[0] : 1.0;
	int bpp = (img->bits_per_pixel + 7) / 8;
	int stride = img->bytes_per_line;

	for (Monitor *m = lock->m; m; m = m->next) {
		for (int yy = 0; yy < m->mh; yy++) {
			unsigned char *row = (unsigned char *)img->data + (m->my + yy) * stride;
			for (int xx = 0; xx < m->mw; xx++) {
				unsigned char *px = row + (m->mx + xx) * bpp;
				double r = px[2] / 255.0;
				double g = px[1] / 255.0;
				double b = px[0] / 255.0;
				double lum = 0.2126 * r + 0.7152 * g + 0.0722 * b;

				unsigned char gr, gg, gb;
				gr = gg = gb = 0;
				gradient_lookup(grad, lum, &gr, &gg, &gb);

				if (blend < 1.0) {
					gr = (unsigned char)(px[2] * (1.0 - blend) + gr * blend);
					gg = (unsigned char)(px[1] * (1.0 - blend) + gg * blend);
					gb = (unsigned char)(px[0] * (1.0 - blend) + gb * blend);
				}
				px[2] = gr;
				px[1] = gg;
				px[0] = gb;
			}
		}
	}

	free(grad->stops);
	free(grad);
}
