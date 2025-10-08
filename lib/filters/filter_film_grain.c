void
filter_film_grain(XImage *img, EffectParams *p, struct lock *lock)
{
    if (!img || !img->data)
        return;

    int strength = (int)p->parameters[0];

    /* bytes per pixel (assuming 24/32 bpp TrueColor) */
    int bpp = img->bits_per_pixel / 8;
    int stride = img->bytes_per_line;

    for (int y = 0; y < img->height; y++) {
        uint8_t *row = (uint8_t *)img->data + y * stride;
        for (int x = 0; x < img->width; x++) {
            uint8_t *px = row + x * bpp;

            /* Generate a random signed offset in [-strength, +strength] */
            int noise = (rand() % (2 * strength + 1)) - strength;

            /* Apply same noise to R, G, B */
            int r = px[2] + noise;
            int g = px[1] + noise;
            int b = px[0] + noise;

            /* Clamp to [0, 255] */
            px[2] = (r < 0) ? 0 : (r > 255) ? 255 : r;
            px[1] = (g < 0) ? 0 : (g > 255) ? 255 : g;
            px[0] = (b < 0) ? 0 : (b > 255) ? 255 : b;
        }
    }
}
