#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#if HAVE_IMLIB
#include <Imlib2.h>
#else
#include <bzlib.h>
#endif

char *
expandhome(const char *string)
{
	int path_length;
	char *ret;

	/* Substitute ~ with home directory */
	if (startswith("~/", string)) {
		const char *home = getenv("HOME");
		if (startswith("/", home)) {
			path_length = strlen(string) + strlen(home);
			ret = calloc(path_length, sizeof(char));
			snprintf(ret, path_length, "%s%s", home, string+1);
			return ret;
		}
	}

	return strdup(string);
}

char *
random_file_from_dir(const char *dirname)
{
	DIR *dir;
	struct dirent *entry;
	char **files = NULL;
	size_t count = 0;

	dir = opendir(dirname);
	if (!dir) {
		perror("opendir");
		return NULL;
	}

	while ((entry = readdir(dir)) != NULL) {
		/* skip "." and ".." */
		if (entry->d_name[0] == '.')
			continue;

		#if !HAVE_IMLIB
		if (!strstr(entry->d_name, ".ff"))
			continue;
		#endif

		files = realloc(files, sizeof(char*) * (count + 1));
		files[count] = strdup(entry->d_name);
		count++;
	}
	closedir(dir);

	if (count == 0) {
		fprintf(stderr, "No image files found in %s\n", dirname);
		free(files);
		return NULL;
	}

	size_t idx = rand() % count;

	char *selected = malloc(strlen(dirname) + strlen(files[idx]) + 2);
	sprintf(selected, "%s/%s", dirname, files[idx]);

	/* cleanup */
	for (size_t i = 0; i < count; i++) free(files[i]);
	free(files);

	return selected;
}

int
load_image_from_string(
	Display *dpy,
	Monitor *m,
	XImage *image,
	const char *file_or_directory,
	BlendOptions *blend_options
) {
	struct stat statbuf;
	int ret = 0;
	char *filename = NULL;
	char *expanded = expandhome(file_or_directory);

	if (stat(expanded, &statbuf) != 0) {
		fprintf(stderr, "error: cannot stat %s\n", expanded);
		goto bail;
	}

	if (S_ISDIR(statbuf.st_mode)) {
		/* If a directory is given, then pick a random file */
		filename = random_file_from_dir(expanded);
		if (!filename) {
			fprintf(stderr, "error: no valid files found in %s\n", expanded);
			goto bail;
		}
	} else {
		filename = strdup(expanded);
	}

	ret = load_image_from_file(dpy, m, image, filename, blend_options);
	free(filename);

bail:
	free(expanded);
	return ret;
}

#if HAVE_IMLIB
int
load_image_from_file(
	Display *dpy,
	Monitor *m,
	XImage *image,
	const char *filename,
	BlendOptions *blend_options
) {
	if (!filename || !strlen(filename))
		return -1;

	Imlib_Image img = imlib_load_image(filename);
	if (!img) {
		fprintf(stderr, "error: failed to load image %s with imlib2\n", filename);
		return -1;
	}

	imlib_context_set_image(img);

	int width = imlib_image_get_width();
	int height = imlib_image_get_height();

	uint32_t *data = imlib_image_get_data();
	if (!data) {
		fprintf(stderr, "error: imlib2 returned NULL data for %s\n", filename);
		imlib_free_image();
		return -1;
	}

	XImage src = {
		.data = (char *)data,
		.width = width,
		.height = height,
		.bits_per_pixel = image->bits_per_pixel,
		.bytes_per_line = width * 4,  /* since imlib2 gives RGBA32 */
	};

	blend_images(image, &src, m, blend_options);
	imlib_free_image();

	return 0;
}
#else  /* No IMLIB, fall back to loading farbfeld image */
typedef struct {
	int is_bz2;
	int fd;
	FILE *f;
	BZFILE *bzf;
	int bzerr;
} FFReader;

int
ff_open(FFReader *r, const char *filename)
{
	r->fd = open(filename, O_RDONLY);
	if (r->fd < 0)
		return -1;

	unsigned char magic[3];
	if (read(r->fd, magic, 3) != 3)
		return -1;
	lseek(r->fd, 0, SEEK_SET); /* rewind */

	r->is_bz2 = (memcmp(magic, "BZh", 3) == 0);
	r->f = NULL;
	r->bzf = NULL;

	if (r->is_bz2) {
		r->f = fdopen(r->fd, "rb");
		if (!r->f)
			return -1;
		r->bzf = BZ2_bzReadOpen(&r->bzerr, r->f, 0, 0, NULL, 0);
		if (r->bzerr != BZ_OK)
			return -1;
	}
	return 0;
}

ssize_t
ff_read(FFReader *r, void *buf, size_t nbytes)
{
	if (r->is_bz2) {
		int n = BZ2_bzRead(&r->bzerr, r->bzf, buf, nbytes);
		if (r->bzerr == BZ_OK || r->bzerr == BZ_STREAM_END)
			return n;
		return -1;
	}

	return read(r->fd, buf, nbytes);
}

void
ff_close(FFReader *r)
{
	if (r->is_bz2) {
		BZ2_bzReadClose(&r->bzerr, r->bzf);
		fclose(r->f); /* also closes r->fd */
	} else {
		close(r->fd);
	}
}

int
load_image_from_file(
	Display *dpy,
	Monitor *m,
	XImage *image,
	const char *filename,
	BlendOptions *blend_options
) {
	if (!filename || !strlen(filename))
		return -1;

	FFReader r;

	if (ff_open(&r, filename) < 0) {
		fprintf(stderr, "error: cannot open %s: %s\n", filename, strerror(errno));
		return -1;
	}

	uint8_t header[16];
	if (ff_read(&r, header, sizeof(header)) != sizeof(header)) {
		fprintf(stderr, "error: failed to read farbfeld header from %s\n", filename);
		ff_close(&r);
		return -1;
	}

	if (memcmp(header, "farbfeld", 8) != 0) {
		fprintf(stderr, "error: %s is not a farbfeld file\n", filename);
		ff_close(&r);
		return -1;
	}

	uint32_t width = (header[8] << 24) | (header[9] << 16) | (header[10] << 8) | header[11];
	uint32_t height = (header[12] << 24) | (header[13] << 16) | (header[14] << 8) | header[15];

	size_t pixels = (size_t)width * height;
	uint8_t *ffbuf = malloc(pixels * 8);
	if (!ffbuf) {
		perror("malloc");
		ff_close(&r);
		return -1;
	}

	if (ff_read(&r, ffbuf, pixels * 8) != (ssize_t)(pixels * 8)) {
		fprintf(stderr, "error: failed to read image data\n");
		free(ffbuf);
		ff_close(&r);
		return -1;
	}
	ff_close(&r);

	XImage src = {
		.data = (char *)ffbuf,
		.width = width,
		.height = height,
		.bits_per_pixel = 64,
		.bytes_per_line = width * 8,  /* since farbfeld gives RGBA64 */
	};

	blend_images(image, &src, m, blend_options);
	free(ffbuf);

	return 0;
}
#endif // HAVE_IMLIB
