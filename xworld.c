#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>

#include <X11/Xlib.h>

#include "vroot.h"
#include "noise.h"

typedef struct Pair {
    int x;
    int y;
} Pair;

static int randRange(int min, int max);
static void shuffle(Pair* array, size_t n);
static void swap(Pair* a, Pair* b);

static int randRange(int min, int max) {
    double scaled = (double)rand() / RAND_MAX;
    return (max - min + 1) * scaled + min;

}

static void shuffle(Pair* array, size_t n) {
    int i, j;
    for(i = n - 1; i > 0; i--) {
        j = rand() % (i + 1);

        swap(&array[i], &array[j]);
    }
}

static void swap(Pair* a, Pair* b) {
    Pair tmp = *a;
    *a = *b;
    *b = tmp;
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
    Pair* coords;
    Pair p;
    double val;

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

    coords = (Pair*)malloc(wa.width * wa.height * sizeof(Pair));

    if(coords == NULL) {
        printf("ERROR HERE\n");
        return -1;
    }

    for(i = 0; i < wa.width; ++i) {
        for(j = 0; j < wa.height; ++j) {
            coords[(i * wa.height) + j].x = i;
            coords[(i * wa.height) + j].y = j;
        }
    }

    while(1) {
        x = rand();
        y = rand();

        shuffle(coords, wa.width * wa.height);

        for(i = 0; i < wa.width; i++) {
            for(j = 0; j < wa.height; j++) {
                p = coords[(i * wa.height) + j];
                val = getNoiseValue(np, (double)(p.x + x) * step, (double)(p.y + y) * step);
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

                XDrawPoint(display, root, g, p.x, p.y);
            }
        }

        XFlush(display);
        usleep(3000000);
    }

    XCloseDisplay(display);
} 
