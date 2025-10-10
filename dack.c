#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <time.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#define CLAMP(V,L,U) (V < L ? L : V > U ? U : V)

/* Xresources preferences */
enum resource_type {
	STRING = 0,
	INTEGER = 1,
	FLOAT = 2
};

typedef struct {
	char *name;
	enum resource_type type;
	void *dst;
} ResourcePref;

typedef struct NamedColor NamedColor;
struct NamedColor {
	char *name;
	char *value;
	unsigned long pixel;
	ResourcePref *resource;
};

typedef struct Monitor Monitor;
struct Monitor {
	int mx, my, mw, mh;  /* monitor size */
	int dpy;
	Monitor *next;
};

struct lock {
	int screen;
	int cmap;
	Window root;
	Display *dpy;
	Monitor *m;
	unsigned int x, y;
	XRectangle *rectangles;
};

#include "lib/include.h"

static void
die(const char *fmt, ...)
{
	va_list ap;
	int saved_errno;

	saved_errno = errno;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':')
		fprintf(stderr, " %s", strerror(saved_errno));
	fputc('\n', stderr);

	exit(1);
}

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if (!(p = calloc(nmemb, size)))
		die("calloc:");
	return p;
}

#include "lib/include.c"

Monitor *
get_monitors(Display *dpy, Window root)
{
	XRRScreenResources *res = XRRGetScreenResourcesCurrent(dpy, root);
	if (!res) {
		fprintf(stderr, "Failed to get XRandR screen resources\n");
		return NULL;
	}

	Monitor *head = NULL;
	Monitor *tail = NULL;

	for (int i = 0; i < res->ncrtc; i++) {
		XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(dpy, res, res->crtcs[i]);
		if (!crtc_info)
			continue;

		/* Skip disabled CRTCs */
		if (crtc_info->width == 0 || crtc_info->height == 0) {
			XRRFreeCrtcInfo(crtc_info);
			continue;
		}

		if (!head) {
			tail = head = ecalloc(1, sizeof(Monitor));
		} else {
			tail = tail->next = ecalloc(1, sizeof(Monitor));
		}

		tail->mx = crtc_info->x;
		tail->my = crtc_info->y;
		tail->mw = crtc_info->width;
		tail->mh = crtc_info->height;
		tail->dpy = i;

		XRRFreeCrtcInfo(crtc_info);
	}

	XRRFreeScreenResources(res);
	return head;
}

void
free_monitors(Monitor *mons)
{
	Monitor *next;

	while (mons) {
		next = mons->next;
		free(mons);
		mons = next;
	}
}

void
free_root_pixmap(Display *dpy, Window root)
{
	Atom props[2] = {
		XInternAtom(dpy, "_XROOTPMAP_ID", False),
		XInternAtom(dpy, "ESETROOT_PMAP_ID", False)
	};

	Pixmap pixmaps[2] = {0, 0};
	int count = 0;

	for (int i = 0; i < 2; i++) {
		Atom type;
		int format;
		unsigned long nitems, bytes_after;
		unsigned char *data = NULL;

		if (XGetWindowProperty(dpy, root, props[i], 0, 1, False, XA_PIXMAP,
		                       &type, &format, &nitems, &bytes_after, &data) == Success) {
			if (data && type == XA_PIXMAP && format == 32 && nitems >= 1) {
				unsigned long *vals = (unsigned long *)data;
				Pixmap p = (Pixmap)vals[0];
				/* Avoid duplicates */
				int already_added = 0;
				for (int j = 0; j < count; j++) {
					if (pixmaps[j] == p) {
						already_added = 1;
						break;
					}
				}
				if (!already_added && count < 2)
					pixmaps[count++] = p;
			}
			if (data)
				XFree(data);
		}
	}

	/* Free unique pixmaps */
	for (int i = 0; i < count; i++) {
		/* This was originally intended to be a call to XFreePixmap to free
		 * the previous background pixmap, but it turns out that when we
		 * make the call to XSetCloseDownMode telling the X server to retain
		 * data, e.g. the pixmap, instead of purging everything when the
		 * connection close, the X server also keeps the display connection
		 * alive despite the call to XCloseDisplay. So while the XFreePixmap
		 * successfully frees the pixmap in question we end up leaking the
		 * previous display connection and we eventually can't start new
		 * windows due to the "Maximum number of clients reached" error.
		 *
		 * As such instead of freeing the pixmap expicitly we make a call
		 * to XKillClient to kill the connection in relation to the previous
		 * pixmap, which also makes the X server free the pixmap.
		 * This solution was found in the feh source code.
		 */
		XKillClient(dpy, pixmaps[i]);
	}

	/* Remove properties */
	for (int i = 0; i < 2; i++)
		XDeleteProperty(dpy, root, props[i]);
}

/* Store the pixmap id (one 32-bit item) in _XROOTPMAP_ID */
void
set_root_pixmap_property(Display *dpy, Window root, Pixmap pmap)
{
	Atom xrootpmap = XInternAtom(dpy, "_XROOTPMAP_ID", False);
	Atom esetroot = XInternAtom(dpy, "ESETROOT_PMAP_ID", False);

	unsigned long p = (unsigned long)pmap; /* use unsigned long for Xlib conversions */

	XChangeProperty(dpy, root, xrootpmap, XA_PIXMAP, 32,
					PropModeReplace, (unsigned char *)&p, 1);
	XChangeProperty(dpy, root, esetroot, XA_PIXMAP, 32,
					PropModeReplace, (unsigned char *)&p, 1);
}

int
main(int argc, char **argv)
{
	int i;
	Display *dpy;

	if (argc == 3 && (!strcmp("-c", argv[1]) || !strcmp("--cfg", argv[1])))
		cmd_filename = argv[2];
	else if (argc != 1)
		die("usage: dack [-c dack.cfg]");

	/* Set random seed, used by filters. */
	time_t t;
	srand((unsigned) time(&t));

	load_config();

	if (!(dpy = XOpenDisplay(NULL)))
		die("dack: cannot open display\n");

	xrdb_init(dpy);

	Window root = DefaultRootWindow(dpy);
	int screen = DefaultScreen(dpy);
	Visual *visual = DefaultVisual(dpy, screen);
	int depth = DefaultDepth(dpy, screen);
	int cmap = DefaultColormap(dpy, screen);
	int root_w = DisplayWidth(dpy, screen);
	int root_h = DisplayHeight(dpy, screen);
	Monitor *mons = get_monitors(dpy, root);

	/* Create an XImage large enough to cover the entire root area, used
	 * for storing wallpapers across monitors and for applying filters */
	XImage *img = XCreateImage(dpy, visual, depth, ZPixmap, 0, NULL, root_w, root_h, 32, 0);

	if (!img) {
		fprintf(stderr, "failed to create XImage\n");
		XCloseDisplay(dpy);
		return 1;
	}

	/* Allocate image data */
	size_t datasize = img->bytes_per_line * img->height;
	img->data = malloc(datasize);
	if (!img->data) {
		fprintf(stderr, "failed to allocate enough memory (%zu) to hold image data\n", datasize);
		XDestroyImage(img);
		XCloseDisplay(dpy);
		return 1;
	}

	/* Allocate colors */
	for (i = 0; i < num_colors; i++) {
		colors[i].pixel = strtopixel(dpy, cmap, colors[i].value);
	}

	/* Set up lock struct, for compatibility with filters. */
	struct lock *lock = ecalloc(1, sizeof(struct lock));

	lock->screen = screen;
	lock->cmap = cmap;
	lock->root = root;
	lock->dpy = dpy;
	lock->m = mons;
	lock->x = root_w;
	lock->y = root_h;

	/* Apply background filters (if any) */
	for (i = 0; i < num_background_filters; i++) {
		if (background_filters[i].func) {
			background_filters[i].func(img, &background_filters[i], lock);
		}
	}

	/* Create the pixmap that we are going to send to the X Server as
	 * the root window background. We copy the image data to the pixmap. */
	Pixmap pmap = XCreatePixmap(dpy, root, root_w, root_h, depth);

	GC gc = XCreateGC(dpy, pmap, 0, NULL);
	XPutImage(dpy, pmap, gc, img, 0, 0, 0, 0, root_w, root_h);

	/* Now use the above pixmap to set the root background */
	XSetWindowBackgroundPixmap(dpy, root, pmap);
	XClearWindow(dpy, root);
	XFlush(dpy);
	XSync(dpy, False);

	/* Free previous root window pixmap, if any. This is to clean up memory
	 * from the previous time we set a wallpaper, as the X server does not do
	 * this automatically when calling XSetWindowBackgroundPixmap. */
	free_root_pixmap(dpy, root);

	/* Save the root window pixmap in an X property so that we can clean up
	 * this memory the next time we change the wallpaper. This is also used
	 * by some compositors like Picom. */
	set_root_pixmap_property(dpy, root, pmap);

	XFreeGC(dpy, gc);
	XDestroyImage(img);

	/* This is needed in order for the X server not to delete the Pixmap used
	 * for the background wallpaper when we close the display connection. */
	XSetCloseDownMode(dpy, RetainPermanent);

	XCloseDisplay(dpy);

	free(lock);
	free_monitors(mons);
	cleanup_config();

	return 0;
}
