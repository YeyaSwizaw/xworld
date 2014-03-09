#include "noise.h"

NoiseParams* defaultNoiseParams(void) {
    NoiseParams* ptr = (NoiseParams*)malloc(sizeof(NoiseParams));
    ptr->octaves = YS_NOISE_OCTAVES;
    ptr->frequency = YS_NOISE_FREQUENCY;
    ptr->lacunarity = YS_NOISE_LACUNARITY;
    ptr->persistence = YS_NOISE_PERSISTENCE;

    return ptr;
}

double getNoiseValue(NoiseParams* np, double x, double y) {
    double value = 0.0;
    double signal = 0.0;
    double currPers = 1.0;
    int currOct = 0;

    x *= np->frequency;
    y *= np->frequency;

    for(; currOct < np->octaves; currOct++) {
        signal = generateCoherentNoise(x, y);
        value += signal * currPers;

        x *= np->lacunarity;
        y *= np->lacunarity;
        currPers *= np->persistence;
    }

    return value;
}

static double generateCoherentNoise(double x, double y) {
    int x0 = (x > 0.0 ? (int)x : (int)(x - 1));
    int x1 = x0 + 1;

    int y0 = (y > 0.0 ? (int)y : (int)(y - 1));
    int y1 = y0 + 1;

    double xd = YS_SCURVE(x - (double)x0);
    double yd = YS_SCURVE(y - (double)y0);

    double x0y0 = generateNoise(x0, y0);
    double x1y0 = generateNoise(x1, y0);
    double x0y1 = generateNoise(x0, y1);
    double x1y1 = generateNoise(x1, y1);

    double v1 = interpolate(x0y0, x1y0, xd);
    double v2 = interpolate(x0y1, x1y1, xd);
        
    return interpolate(v1, v2, yd);
}

static double generateNoise(int x, int y) {
    int n = (x * 157) + (y * 2633);
    n = (n << 13) ^ n;
    return (1.0 - ((n * (n * n * 15731 + 789221) + 1376312579) & 0x7fffffff) / 1073741824.0);    
}

static double interpolate(double v1, double v2, double a) {
    return ((1.0 - a) * v1) + (a * v2);
}

static double randRange(double min, double max) {
    double range = max - min;
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

int main(int argc, char* argv[]) {
    NoiseParams* np = defaultNoiseParams();
    double i, j, x, y, val;

    srand(time(NULL));
    x = randRange(0, 1000.0);
    y = randRange(0, 1000.0);

    for(i = x; i < (x + 3); i += 0.05) {
        for(j = y; j < (y + 9); j += 0.05) {
            val = getNoiseValue(np, i, j);
            if(val < -0.3) {
                printf("~");
            } else if(val < 0.4) {
                printf("#");
            } else {
                printf("^");
            }
        }
        printf("\n");
    }

    free(np);

    return 0;
}
