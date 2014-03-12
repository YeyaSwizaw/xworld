/* xworld - Copyright (C) 2014 Samuel Sleight 
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include <stdlib.h>

#include "screenhack.h"

#define XWORLD_DEFAULT_NOISE_SEED 0;
#define XWORLD_DEFAULT_NOISE_OCTAVES 6;
#define XWORLD_DEFAULT_NOISE_FREQUENCY 1.0;
#define XWORLD_DEFAULT_NOISE_LACUNARITY 2.0;
#define XWORLD_DEFAULT_NOISE_PERSISTENCE 0.5;

#define xworld_scurve(a) ((a) * (a) * (3.0 - 2.0 * (a)))

/* Paramater struct for noise function */
typedef struct xworld_noise_param {
    int seed;
    int octaves;
    double frequency;
    double lacunarity;
    double persistence;
} xworld_noise_param;

/* coordinates */
typedef struct xworld_coord {
    int x;
    int y;
} xworld_coord;

/* State object */
typedef struct xworld_state {
    Display* dpy;
    Window wnd;
    XWindowAttributes wa;

    GC gc;
    XColor water, sand, peak, mtn, grass;
    double w, s, m, g;

    xworld_noise_param* np;
    xworld_coord* coords;

    int ppl;
    int delay;
    double step;

    int coord;
} xworld_state;

/* Function prototypes */
static void xworld_shuffle(xworld_coord* array, size_t n);
static void xworld_swap(xworld_coord* a, xworld_coord* b);
static xworld_noise_param* xworld_default_noise(void);
static double xworld_get_noise_value(xworld_noise_param* np, double x, double y);
static double xworld_gen_coherent_noise(double x, double y, int seed);
static double xworld_gen_noise(int x, int y, int seed);
static double xworld_interpolate(double v1, double v2, double a);
static void* xworld_init(Display* dpy, Window wnd);
static unsigned long xworld_draw(Display* dpy, Window wnd, void* state);
static void xworld_reshape(Display* dpy, Window wnd, void* state, unsigned int w, unsigned int h);
static Bool xworld_event (Display *dpy, Window wnd, void *state, XEvent *event);
static void xworld_free(Display* dpy, Window wnd, void* state);

/* Shuffle an array of coordinates 
 * used to randomise the drawing of pixels */
static void xworld_shuffle(xworld_coord* array, size_t n) {
    int i, j;
    for(i = n - 1; i > 0; --i) {
        j = random() % (i + 1);
        xworld_swap(&array[i], &array[j]);
    }
}

static void xworld_swap(xworld_coord* a, xworld_coord* b) {
    xworld_coord tmp = *a;
    *a = *b;
    *b = tmp;
}

/*
 * Perlin noise
 */

/* Get default noise parameters */
static xworld_noise_param* xworld_default_noise(void) {
    xworld_noise_param* ptr = (xworld_noise_param*)calloc(1, sizeof(xworld_noise_param));
    ptr->seed = XWORLD_DEFAULT_NOISE_SEED;
    ptr->octaves = XWORLD_DEFAULT_NOISE_OCTAVES;
    ptr->frequency = XWORLD_DEFAULT_NOISE_FREQUENCY;
    ptr->lacunarity = XWORLD_DEFAULT_NOISE_LACUNARITY;
    ptr->persistence = XWORLD_DEFAULT_NOISE_PERSISTENCE;
    return ptr;
}

/* Get the noise value at a location with the passed params */
static double xworld_get_noise_value(xworld_noise_param* np, double x, double y) {
    int seed;
    int oct = 0;
    double val = 0.0;
    double pers = 1.0;

    x *= np->frequency;
    y *= np->frequency;

    for(; oct < np->octaves; ++oct) {
        seed = (np->seed + oct) & 0xffffffff;
        val += xworld_gen_coherent_noise(x, y, seed) * pers;

        x *= np->lacunarity;
        y *= np->lacunarity;
        pers *= np->persistence;
    }

    return val;
}

static double xworld_gen_coherent_noise(double x, double y, int seed) {
    int x0 = (x > 0.0 ? (int)x : (int)(x - 1));
    int x1 = x0 + 1;

    int y0 = (y > 0.0 ? (int)y : (int)(y - 1));
    int y1 = y0 + 1;

    double xd = xworld_scurve(x - (double)x0);
    double yd = xworld_scurve(y - (double)y0);

    double x0y0 = xworld_gen_noise(x0, y0, seed);
    double x1y0 = xworld_gen_noise(x1, y0, seed);
    double x0y1 = xworld_gen_noise(x0, y1, seed);
    double x1y1 = xworld_gen_noise(x1, y1, seed);

    double v1 = xworld_interpolate(x0y0, x1y0, xd);
    double v2 = xworld_interpolate(x0y1, x1y1, xd);
        
    return xworld_interpolate(v1, v2, yd);
}

static double xworld_gen_noise(int x, int y, int seed) {
    int n = ((x * 157) + (y * 31337) + (seed * 2633)) & 0x7fffffff;
    n = (n << 13) ^ n;
    return (1.0 - ((n * (n * n * 15731 + 789221) + 1376312579) & 0x7fffffff) / 1073741824.0);    
}

static double xworld_interpolate(double v1, double v2, double a) {
    return ((1.0 - a) * v1) + (a * v2);
}

/*
 * xworld 
 */
/* Default resource values */
static const char *xworld_defaults[] = { 
    ".background:   #000000",
    ".foreground:   #FFFFFF",
    "*ppl:          100",
    "*delay:        3000000",
    "*gridstep:     0.0075",

    "*watercol:     #0000FF",
    "*sandcol:      #FFCC00",
    "*peakcol:      #EEEEEE",
    "*mtncol:       #AAAAAA",
    "*grasscol:     #00FF00",

    "*watermax:     30",
    "*sandmax:      35",
    "*grassmax:     70",
    "*mtnmax:       85",
    0
};

/* Command line options */
static XrmOptionDescRec xworld_options[] = { 
    { "-pixels-per-loop",   ".ppl",         XrmoptionSepArg,    0 },
    { "-delay",             ".delay",       XrmoptionSepArg,    0 },
    { "-grid-step",         ".gridstep",    XrmoptionSepArg,    0 },
    { "-water-col",         ".waterhex",    XrmoptionSepArg,    0 },
    { "-sand-col",          ".sandhex",     XrmoptionSepArg,    0 },
    { "-peak-col",          ".peakhex",     XrmoptionSepArg,    0 },
    { "-mtn-col",           ".mtnhex",      XrmoptionSepArg,    0 },
    { "-grass-col",         ".grasshex",    XrmoptionSepArg,    0 },
    { "-water-max",         ".watermax",    XrmoptionSepArg,    0 },
    { "-sand-max",          ".sandmax",     XrmoptionSepArg,    0 },
    { "-mtn-max",           ".mtnmax",      XrmoptionSepArg,    0 },
    { "-grass-max",         ".grassmax",    XrmoptionSepArg,    0 },
    { 0, 0, 0, 0 } 
};

/* Initialise the state */
static void* xworld_init(Display* dpy, Window wnd) {
    xworld_state* st = (xworld_state*)calloc(1, sizeof(*st));
    Colormap cmap;
    int screen, i, j;

    st->np = xworld_default_noise();

    st->dpy = dpy;
    st->wnd = wnd;
    XGetWindowAttributes(st->dpy, st->wnd, &(st->wa));

    screen = DefaultScreen(st->dpy);
    cmap = XDefaultColormap(st->dpy, screen);

    st->gc = XCreateGC(st->dpy, st->wnd, screen, NULL);

    XParseColor(st->dpy, cmap, 
            get_string_resource(st->dpy, "watercol", "String"), &(st->water));
    XAllocColor(st->dpy, cmap, &(st->water));

    XParseColor(st->dpy, cmap, 
            get_string_resource(st->dpy, "sandcol", "String"), &(st->sand));
    XAllocColor(st->dpy, cmap, &(st->sand));

    XParseColor(st->dpy, cmap, 
            get_string_resource(st->dpy, "peakcol", "String"), &(st->peak));
    XAllocColor(st->dpy, cmap, &(st->peak));

    XParseColor(st->dpy, cmap, 
            get_string_resource(st->dpy, "mtncol", "String"), &(st->mtn));
    XAllocColor(st->dpy, cmap, &(st->mtn));

    XParseColor(st->dpy, cmap, 
            get_string_resource(st->dpy, "grasscol", "String"), &(st->grass));
    XAllocColor(st->dpy, cmap, &(st->grass));

    st->w = (get_float_resource(st->dpy, "watermax", "Float") / 50.0) - 1;
    st->s = (get_float_resource(st->dpy, "sandmax", "Float") / 50.0) - 1;
    st->g = (get_float_resource(st->dpy, "grassmax", "Float") / 50.0) - 1;
    st->m = (get_float_resource(st->dpy, "mtnmax", "Float") / 50.0) - 1;

    st->ppl = get_integer_resource(st->dpy, "ppl", "Integer");
    st->delay = get_integer_resource(st->dpy, "delay", "Integer");
    st->step = get_float_resource(st->dpy, "gridstep", "Float");

    st->coords = (xworld_coord*)calloc(1, st->wa.height * st->wa.width * sizeof(*(st->coords)));
    for(i = 0; i < st->wa.width; ++i) {
        for(j = 0; j < st->wa.height; ++j) {
            st->coords[(i * st->wa.height) + j].x = i;
            st->coords[(i * st->wa.height) + j].y = j;
        } 
    } 

    st->coord = st->wa.width * st->wa.height;

    return st;
}

static unsigned long xworld_draw(Display* dpy, Window wnd, void* state) {
    xworld_state* st = (xworld_state*)state;
    xworld_coord c;
    double val;
    int threshold = st->wa.width * st->wa.height;
    int i;

    /* World is rendered - reset and wait */
    if(st->coord >= st->wa.width * st->wa.height) {
        st->coord = 0;
        st->np->seed = random();
        xworld_shuffle(st->coords, threshold);

        return st->delay;
    }

    /* Draw pixels */
    for(i = 0; i < st->ppl; ++i) {
        c = st->coords[st->coord];
        val = xworld_get_noise_value(st->np, c.x * st->step, c.y * st->step);
        st->coord++;

        if(val < st->w) {
            XSetForeground(st->dpy, st->gc, st->water.pixel);
        } else if(val < st->s) {
            XSetForeground(st->dpy, st->gc, st->sand.pixel);
        } else if(val < st->g) {
            XSetForeground(st->dpy, st->gc, st->grass.pixel);
        } else if(val < st->m) {
            XSetForeground(st->dpy, st->gc, st->mtn.pixel);
        } else {
            XSetForeground(st->dpy, st->gc, st->peak.pixel);
        }

        XDrawPoint(st->dpy, st->wnd, st->gc, c.x, c.y);

        if(st->coord >= threshold) {
            break;
        }
    }

    return 0;
}

static void xworld_reshape(Display* dpy, Window wnd, void* state, unsigned int w, unsigned int h) {

}

static Bool xworld_event(Display* dpy, Window wnd, void* state, XEvent* event) {
    return False;
}

static void xworld_free(Display* dpy, Window wnd, void* state) {
    xworld_state* st = (xworld_state*)state;
    XFreeGC(st->dpy, st->gc);
    free(st->np);
    free(st->coords);
    free(st);
}

XSCREENSAVER_MODULE ("XWorld", xworld)
