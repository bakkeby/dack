void
filter_invert(XImage *img, EffectParams *p, struct lock *lock)
{
    int width  = img->width;
    int height = img->height;
    int bpp    = img->bits_per_pixel / 8;   // 4
    int stride = img->bytes_per_line;

    uint8_t *data = (uint8_t *)img->data;

    for (int y = 0; y < height; y++) {
        uint8_t *row = data + y * stride;
        for (int x = 0; x < width; x++) {
            uint8_t *p = row + x * bpp;

            p[0] = 255 - p[0];  // Blue
            p[1] = 255 - p[1];  // Green
            p[2] = 255 - p[2];  // Red
            // Leave p[3] (padding/alpha) unchanged
        }
    }
}
