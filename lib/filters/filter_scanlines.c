void
filter_scanlines(XImage *img, EffectParams *p, struct lock *lock)
{
    int spacing = p->parameters[0];
    float intensity = (float)p->parameters[1];

    int width  = img->width;
    int height = img->height;
    int bpp    = img->bits_per_pixel / 8;
    int stride = img->bytes_per_line;
    uint8_t *data = (uint8_t *)img->data;

    for (int y = 0; y < height; y += spacing) {
        uint8_t *row = data + y * stride;
        for (int x = 0; x < width; x++) {
            uint8_t *p = row + x * bpp;
            p[0] = (uint8_t)(p[0] * intensity); // B
            p[1] = (uint8_t)(p[1] * intensity); // G
            p[2] = (uint8_t)(p[2] * intensity); // R
        }
    }
}