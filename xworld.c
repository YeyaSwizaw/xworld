#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>

#include <X11/Xlib.h>

#include "vroot.h"
#include "noise.h"

static int randRange(int min, int max);

static int randRange(int min, int max) {
    double scaled = (double)rand() / RAND_MAX;
    return (max - min + 1) * scaled + min;

}

int main(int argc, char* argv[]) {
    NoiseParams* np = defaultNoiseParams();
    double step = 0.0075;
    Display *display;
    Window root;
    XWindowAttributes wa;
    int screen;
    Colormap cmap;
    GC g;
    XColor blue, yellow, white, grey, dgreen, green;
    int i, j, x, y;

    srand(time(NULL));

    display = XOpenDisplay(getenv("DISPLAY"));

    root = DefaultRootWindow(display);
    XGetWindowAttributes(display, root, &wa);

    screen = DefaultScreen(display);
    cmap = XDefaultColormap(display, screen);

    g = XCreateGC(display, root, screen, NULL);

    XParseColor(display, cmap, "#0000FF", &blue);
    XAllocColor(display, cmap, &blue);

    XParseColor(display, cmap, "#FFCC00", &yellow);
    XAllocColor(display, cmap, &yellow);

    XParseColor(display, cmap, "#EEEEEE", &white);
    XAllocColor(display, cmap, &white);

    XParseColor(display, cmap, "#AAAAAA", &grey);
    XAllocColor(display, cmap, &grey);

    XParseColor(display, cmap, "#008800", &dgreen);
    XAllocColor(display, cmap, &dgreen);

    XParseColor(display, cmap, "#00FF00", &green);
    XAllocColor(display, cmap, &green);

    x = rand();
    y = rand();

    while(1) {
        for(i = 0; i < wa.width; i++) {
            for(j = 0; j < wa.height; j++) {
                double val = getNoiseValue(np, (double)(i + x) * step, (double)(j + y) * step);
                if(val < -0.4) {
                    XSetForeground(display, g, blue.pixel);
                } else if(val < -0.35) {
                    XSetForeground(display, g, yellow.pixel);
                } else if(val < 0.5) {
                    XSetForeground(display, g, green.pixel);
                } else if(val < 0.7) {
                    XSetForeground(display, g, grey.pixel);
                } else {
                    XSetForeground(display, g, white.pixel);
                }

                XDrawPoint(display, root, g, i, j);
            }
        }

        XFlush(display);
        usleep(100);
    }

    XCloseDisplay(display);
} 
