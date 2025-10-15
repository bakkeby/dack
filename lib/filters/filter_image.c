void
filter_screenshot(XImage *img, EffectParams *p, struct lock *lock)
{
	XImage *rootimg = XGetImage(lock->dpy, lock->root, 0, 0,
								lock->x, lock->y, AllPlanes, ZPixmap);
	memcpy(img->data, rootimg->data, img->bytes_per_line * img->height);
	XDestroyImage(rootimg);
}

void
filter_wallpaper(XImage *img, EffectParams *p, struct lock *lock)
{
	Monitor *m;
	int idx;
	const char *path;

	if (!p->num_string_parameters)
		return;

	float blend_strength = (float)(p->parameters[0] ? p->parameters[0] : 1.0);
	blend_strength = CLAMP(blend_strength, 0.0, 1.0);
	int blend_mode = (int)(p->parameters[1]);


	BlendOptions options = {
		.blend_mode = blend_mode,
		.blend_strength = blend_strength,
		.blend_position = CENTER,
	};


	for (m = lock->m, idx = 0; m; m = m->next, idx++) {
		path = p->string_parameters[idx % p->num_string_parameters];
		load_image_from_string(lock->dpy, m, img, path, &options);
	}
}

static Pixmap
get_root_pixmap(Display *dpy, Window root)
{
	Atom props[2];
	props[0] = XInternAtom(dpy, "_XROOTPMAP_ID", False);
	props[1] = XInternAtom(dpy, "ESETROOT_PMAP_ID", False);

	for (int i = 0; i < 2; i++) {
		Atom type;
		int format;
		unsigned long nitems, bytes_after;
		unsigned char *data = NULL;
		Pixmap pmap = 0;

		if (XGetWindowProperty(dpy, root, props[i], 0, 1, False, XA_PIXMAP,
							   &type, &format, &nitems, &bytes_after, &data) == Success) {
			if (data && type == XA_PIXMAP && format == 32 && nitems >= 1)
				pmap = *(Pixmap *)data;
			if (data)
				XFree(data);
			if (pmap)
				return pmap;
		}
	}
	return 0;
}

void
filter_root_wallpaper(XImage *img, EffectParams *p, struct lock *lock)
{
	Pixmap pmap = get_root_pixmap(lock->dpy, lock->root);

	if (!pmap)
		return;

	Window root_return;
	int x, y;
	unsigned int width, height, border_width, depth;

	/* As a precaution look up the size of the root background wallpaper
	 * and make sure that this matches the dimensions of the entire screen
	 * area. This should normally match, but may not if the screen resolution
	 * or the number of monitors have changed. */
	if (!XGetGeometry(lock->dpy, pmap, &root_return, &x, &y, &width, &height, &border_width, &depth))
		return;

	if (width != lock->x || height != lock->y)
		return;

	XImage *rootimg = XGetImage(lock->dpy, pmap, 0, 0, lock->x, lock->y, AllPlanes, ZPixmap);
	memcpy(img->data, rootimg->data, img->bytes_per_line * img->height);
	XDestroyImage(rootimg);
}

void
filter_image(XImage *img, EffectParams *p, struct lock *lock)
{
	Monitor *m;
	int idx;

	if (!p->num_string_parameters)
		return;

	int target_monitor = p->parameters[0];
	float blend_strength = (float)(p->parameters[1] ? p->parameters[1] : 1.0);
	blend_strength = CLAMP(blend_strength, 0.0, 1.0);
	int blend_mode = (int)(p->parameters[2]);
	int blend_position = (int)(p->parameters[3]);
	int rel_x = (int)(p->parameters[4]);
	int rel_y = (int)(p->parameters[5]);

	BlendOptions options = {
		.blend_mode = blend_mode,
		.blend_strength = blend_strength,
		.blend_position = blend_position,
		.x = rel_x,
		.y = rel_y,
	};

	for (m = lock->m, idx = 0; m; m = m->next, idx++) {
		if (idx != target_monitor)
			continue;

		load_image_from_string(lock->dpy, m, img, p->string_parameters[0], &options);
	}
}

void
filter_mask(XImage *img, EffectParams *p, struct lock *lock)
{
	Monitor *m;
	int idx;

	if (!p->num_string_parameters)
		return;

	float blend_strength = (float)(p->parameters[0] ? CLAMP(p->parameters[0], 0.0f, 1.0f) : 1.0);
	int blend_mode = (int)(p->parameters[1] ? p->parameters[1] : BLEND_OVERLAY);

	BlendOptions options = {
		.blend_mode = blend_mode,
		.blend_strength = blend_strength,
		.blend_position = TILE,
	};

	for (m = lock->m, idx = 0; m; m = m->next, idx++) {
		load_image_from_string(lock->dpy, m, img, p->string_parameters[0], &options);
	}
}
