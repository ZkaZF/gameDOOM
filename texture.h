#ifndef TEXTURE_H
#define TEXTURE_H

/*
 * texture.h — Color and shading utility functions
 * 
 * Provides helper functions for procedural color generation
 * and shading calculations.
 */

/* Clamp a float value between 0 and 1 */
static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* Linear interpolation */
static float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

/* Apply fog to a color based on distance */
static void applyFog(float* r, float* g, float* b, float distance, float maxDist) {
    float fogFactor = clampf(1.0f - (distance / maxDist), 0.1f, 1.0f);
    *r *= fogFactor;
    *g *= fogFactor;
    *b *= fogFactor;
}

#endif /* TEXTURE_H */
