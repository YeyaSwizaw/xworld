#ifndef YS_NOISE_H
#define YS_NOISE_H

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/*
 * Adapted from the libnoise source (http://libnoise.sourceforge.net/)
 * and from http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
 */

#define YS_NOISE_OCTAVES 6
#define YS_NOISE_FREQUENCY 1.0
#define YS_NOISE_LACUNARITY 2.0
#define YS_NOISE_PERSISTENCE 0.5

#define YS_SCURVE(a) ((a) * (a) * (3.0 - 2.0 * (a)))

typedef struct NoiseParams {
    int octaves;
    double frequency;
    double lacunarity;
    double persistence;
} NoiseParams;

NoiseParams* defaultNoiseParams(void);
double getNoiseValue(NoiseParams* np, double x, double y);

#endif 
