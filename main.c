#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct MandelbrotOptions {
    Display *d;
    Window *w;
    long double zoom;
    uint64_t max_iter;
    uint64_t width;
    uint64_t height;
    uint64_t stepx;
    uint64_t stepy;
    uint64_t off_x;
    uint64_t off_y;
};

#define _C(b) (b + (b << 8) + (b << 16))

void draw_mandelbrot(struct MandelbrotOptions *opts) {

    for (uint64_t cx = 0; cx < opts->width; cx++) {
        for (uint64_t cy = 0; cy < opts->height; cy++) {
            const long double sx = (3.5f * (opts->off_x + (double)cx / opts->zoom) / opts->width) - 2.5f;
            const long double sy = (2.0f * (opts->off_y + (double)cy / opts->zoom) / opts->height) - 1.0f;

            uint64_t iter = 0;
            for (long double x = 0, y = 0; (x * x) + (y * y) <= 4.0f && iter < opts->max_iter; iter++) {
                const long double tmpx = (x * x) - (y * y) + sx;
                y = (2 * x * y) + sy;
                x = tmpx;
            }

            uint8_t v = (255 - (255.0f * (double)iter / (double)opts->max_iter));
            XSetForeground(opts->d, DefaultGC(opts->d, DefaultScreen(opts->d)), _C(v));
            XFillRectangle(opts->d, *(opts->w), DefaultGC(opts->d, DefaultScreen(opts->d)), cx, cy, 1, 1);

        }
    }

}
 
int main(int argc, char **argv) {
    Display *d;
    Window w;
    XEvent e;
    const char *msg = "Hello, World!";
    int s;
 
    d = XOpenDisplay(NULL);
    if (d == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
 
    s = DefaultScreen(d);
    w = XCreateSimpleWindow(d, RootWindow(d, s), 10, 10, 1150, 600, 1, BlackPixel(d, s), WhitePixel(d, s));
    XSelectInput(d, w, ExposureMask | KeyPressMask);
    XMapWindow(d, w);

    XWindowAttributes wa;
    XGetWindowAttributes(d, w, &wa);
    const uint64_t width = wa.width;
    const uint64_t height = wa.height;

    struct MandelbrotOptions o = { 
        .d = d, 
        .w = &w, 
        .zoom = 1, 
        .max_iter = 100, 
        .width = width, 
        .height = height, 
        .stepx = width / 25.0,
        .stepy = height / 25.0,
        .off_x = 0,
        .off_y = 0 
    };
 
    while (1) {
        XNextEvent(d, &e);
        if (e.type == Expose) {
            draw_mandelbrot(&o);
        }
        if (e.type == KeyPress) {
            switch (e.xkey.keycode) {
                case 20: // -
                    o.zoom /= 1.25f;
                    break;

                case 21: // +
                    o.zoom *= 1.25f;
                    break;

                case 24: // q
                    goto end;

                case 38: // a
                    o.max_iter -= 25;
                    break;

                case 39: // s
                    o.max_iter += 25;
                    break;

                case 111: // UP_ARROW
                    o.off_y -= (o.stepy / o.zoom);
                    o.off_y == o.off_y ? o.off_y : 1;
                    break;

                case 113: // LEFT_ARROW
                    o.off_x -= (o.stepx / o.zoom);
                    o.off_x == o.off_x ? o.off_x : 1;
                    break;

                case 114: // RIGHT_ARROW
                    o.off_x += (o.stepx / o.zoom);
                    break;

                case 116: // DOWN_ARROW
                    o.off_y += (o.stepy / o.zoom);
                    break;
            };

            draw_mandelbrot(&o);

        }
    }

end:
    XCloseDisplay(d);
    return 0;
}