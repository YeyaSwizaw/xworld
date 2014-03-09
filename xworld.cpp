#include <algorithm>
#include <random>
#include <string>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

#include <wg/eworld.hpp>

#include <X11/Xlib.h>

extern "C" {

#include "vroot.h"

} // extern "C";

int main(int argc, char* argv[]) {
    srand(time(NULL));

    /* X Initialising */
    Display *display;
    Window root;
    XWindowAttributes wa;
    int screen;
    Colormap cmap;
    GC g;
    XColor blue, yellow, white, grey, dgreen, green;

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

    /* WorldGen initialising */
    wg::EnhancedWorld<unsigned long>* w = (new wg::EnhancedWorld<unsigned long>)
        ->setChunkSize(wa.width, wa.height);

    wg::RandomNoiseMap* heightmap = w->addRandomNoiseMap()
        ->setGridSize(0.01 * ((double)wa.height / (double)wa.width));

    wg::RandomNoiseMap* random = w->addRandomNoiseMap()
        ->setGridSize(0.04 * ((double)wa.height / (double)wa.width));

    wg::RandomNoiseMap* climate = w->addRandomNoiseMap()
        ->setGridSize(0.02 * ((double)wa.height / (double)wa.width));

	// Water
	w->addTileDefinition(blue.pixel)
		->addConstraint({heightmap, wg::ConstraintType::LT, -0.3});

	// Beach
	w->addTileDefinition(yellow.pixel)
		->addConstraint({heightmap, wg::ConstraintType::LT, -0.2});

	// High Mountains
	w->addTileDefinition(white.pixel)
		->addConstraint({heightmap, wg::ConstraintType::GT, 0.6})
		->addConstraint({random, wg::ConstraintType::GT, 0.2});

	// Mountains
	w->addTileDefinition(grey.pixel)
		->addConstraint({heightmap, wg::ConstraintType::GT, 0.45});

	// Forest
	w->addTileDefinition(dgreen.pixel)
		->addConstraint({heightmap, wg::ConstraintType::GT, -0.1})
		->addConstraint({climate, wg::ConstraintType::GT, 0.2});

	// Desert
	w->addTileDefinition(yellow.pixel)
		->addConstraint({heightmap, wg::ConstraintType::GT, -0.1})
		->addConstraint({climate, wg::ConstraintType::LT, -0.45});

	// Plains
	w->addTileDefinition(green.pixel);

    std::random_device rd;
    std::mt19937 gen(rd());

    std::vector<std::pair<int, int>> pairs;

    while(true) {
        heightmap->setSeed(std::to_string(rand()));
        random->setSeed(std::to_string(rand()));
        climate->setSeed(std::to_string(rand()));

        w->generate(0, 0);

        pairs.clear();

        for(int i = 0; i < wa.width; i++) {
            for(int j = 0; j < wa.height; j++) {
                pairs.push_back({i, j});

            } // for(int j = 0; j < wa.height; j++);
    
        } // for(int i = 0; i < wa.width; i++);

        std::shuffle(std::begin(pairs), std::end(pairs), gen);

        for(auto& p : pairs) {
            XSetForeground(display, g, w->getObject(p.first, p.second));
            XDrawPoint(display, root, g, p.first, p.second);

        } // for(auto& p : pairs);

        XFlush(display);
        usleep(100);

    } // while(true);

    XCloseDisplay(display);

} // int main(int argc, char* argv[]);
