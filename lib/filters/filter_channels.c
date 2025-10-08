void
filter_channels(XImage *img, EffectParams *p, struct lock *lock)
{
    if (!img || !img->data) return;

    int keep_r = (int)p->parameters[0]; /* 1 = keep, 0 = remove */
    int keep_g = (int)p->parameters[1];
    int keep_b = (int)p->parameters[2];

    int bpp = img->bits_per_pixel / 8;
    if (bpp < 3) return; /* must have RGB */

    int stride = img->bytes_per_line;

    for (int y = 0; y < img->height; y++) {
        uint8_t *row = (uint8_t*)img->data + y * stride;
        for (int x = 0; x < img->width; x++) {
            uint8_t *px = row + x * bpp;

            if (!keep_b) px[0] = 0;
            if (!keep_g) px[1] = 0;
            if (!keep_r) px[2] = 0;

            if (bpp == 4) px[3] = 0xff; /* keep alpha opaque */
        }
    }
}
