/* -------------------------------------------------------------
 *    grayscale filter
 *    -------------------------------------------------------------
 *    Parameters
 *       XImage *img – image returned by XGetImage().
 *    What it does:
 *        - Walks through every pixel of the image.
 *        - Computes a luminance value using the Rec. 601 coefficients
 *          (R × 0.299 + G × 0.587 + B × 0.114). Those weights give a
 *          perceptually balanced gray that matches what most viewers
 *          expect. Optionally the R, G and B coefficients can be passed
 *          as arguments.
 *        - Replaces the R, G and B components with that luminance.
 *          If the image has an extra alpha byte (32‑bpp) the alpha is
 *          left untouched.
*/
void
filter_grayscale(XImage *img, EffectParams *p, struct lock *lock)
{
	int x, y;

	float red_coefficient = (float)(p->parameters[0] ? p->parameters[0] : 0.299f);
	float green_coefficient = (float)(p->parameters[1] ? p->parameters[1] : 0.587f);
	float blue_coefficient = (float)(p->parameters[2] ? p->parameters[2] : 0.114f);

	if (!img)
		return;

	/* Bytes per pixel – works for 24‑bpp (3) and 32‑bpp (4). */
	int bpp = img->bits_per_pixel / 8;
	if (bpp < 3)
		return; /* we need at least RGB */

	/* Walk the image row‑by‑row */
	for (y = 0; y < img->height; ++y) {
		unsigned char *row = (unsigned char *)img->data +
							 y * img->bytes_per_line;

		for (x = 0; x < img->width; ++x) {
			unsigned char *pix = row + x * bpp;

			/* XImage stores pixels in native byte order:
			   most X servers use little‑endian, so the order is
			   B G R (and possibly A).  To be safe we read the
			   three colour bytes individually. */
			unsigned char blue  = pix[0];
			unsigned char green = pix[1];
			unsigned char red   = pix[2];

			/* Rec. 601 luma = 0.299 R + 0.587 G + 0.114 B */
			unsigned char gray = (unsigned char)(
				red_coefficient * red +
				green_coefficient * green +
				blue_coefficient * blue + 0.5f);   /* +0.5 for rounding */

			/* Write the gray value back to all colour channels */
			pix[0] = gray;   /* B */
			pix[1] = gray;   /* G */
			pix[2] = gray;   /* R */
			/* Alpha (pix[3]) is left unchanged if it exists */
		}
	}
}
