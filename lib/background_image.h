static char *expandhome(const char *string);
static void create_lock_image(Display *dpy, struct lock *lock);
static void cleanup_lock_image(Display *dpy);
static int load_image_from_string(Display *dpy, Monitor *m, XImage *image, const char *file_or_directory, float blend);
static int load_lock_image_from_file(Display *dpy, const char *background_image);
static int load_image_from_file(Display *dpy, Monitor *m, XImage *image, const char *filename, float blend);
