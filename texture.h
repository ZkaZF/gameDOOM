#ifndef TEXTURE_H
#define TEXTURE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>



static float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

static void applyFog(float* r, float* g, float* b, float distance, float maxDist) {
    float fogFactor = clampf(1.0f - (distance / maxDist), 0.1f, 1.0f);
    *r *= fogFactor;
    *g *= fogFactor;
    *b *= fogFactor;
}

typedef struct {
    unsigned char *pixels; 
    int width;
    int height;
} BmpTexture;

static int loadBMP(const char *filename, BmpTexture *tex) {
    FILE *f;
    unsigned char header[54];
    int dataOffset, width, height, rowSize, row;
    unsigned char *rawRow;
    f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "[texture] Cannot open '%s'\n", filename);
        return 0;
    }
    
    if (fread(header, 1, 54, f) != 54 ||
        header[0] != 'B' || header[1] != 'M') {
        fprintf(stderr, "[texture] '%s' is not a valid BMP file\n", filename);
        fclose(f);
        return 0;
    }
    dataOffset = *(int*)&header[10];
    width      = *(int*)&header[18];
    height     = *(int*)&header[22];
    
    if (*(short*)&header[28] != 24) {
        fprintf(stderr, "[texture] '%s' is not a 24-bit BMP\n", filename);
        fclose(f);
        return 0;
    }
    
    rowSize = ((width * 3 + 3) / 4) * 4;
    tex->width  = width;
    tex->height = height;
    tex->pixels = (unsigned char*)malloc(width * height * 3);
    if (!tex->pixels) {
        fclose(f);
        return 0;
    }
    rawRow = (unsigned char*)malloc(rowSize);
    if (!rawRow) { free(tex->pixels); fclose(f); return 0; }
    fseek(f, dataOffset, SEEK_SET);
    
    for (row = height - 1; row >= 0; row--) {
        int col;
        fread(rawRow, 1, rowSize, f);
        for (col = 0; col < width; col++) {
            tex->pixels[(row * width + col) * 3 + 0] = rawRow[col * 3 + 2]; 
            tex->pixels[(row * width + col) * 3 + 1] = rawRow[col * 3 + 1]; 
            tex->pixels[(row * width + col) * 3 + 2] = rawRow[col * 3 + 0]; 
        }
    }
    free(rawRow);
    fclose(f);
    printf("[texture] Loaded '%s' (%dx%d)\n", filename, width, height);
    return 1;
}

static void sampleBmpTile(const BmpTexture *tex,
                           float wx, float wy,
                           float tileScale,
                           float *r, float *g, float *b)
{
    int tx, ty;
    unsigned char *p;
    if (!tex->pixels || tex->width <= 0 || tex->height <= 0) {
        *r = *g = *b = 0.5f;
        return;
    }
    
    float u = wx / tileScale;
    float v = wy / tileScale;
    
    u = u - (float)(int)u;
    v = v - (float)(int)v;
    if (u < 0.0f) u += 1.0f;
    if (v < 0.0f) v += 1.0f;
    tx = (int)(u * tex->width)  % tex->width;
    ty = (int)(v * tex->height) % tex->height;
    p = &tex->pixels[(ty * tex->width + tx) * 3];
    *r = p[0] / 255.0f;
    *g = p[1] / 255.0f;
    *b = p[2] / 255.0f;
}

static BmpTexture gFloorTex = {NULL, 0, 0};

static void initFloorTexture(void) {
    if (!loadBMP("Floor.bmp", &gFloorTex)) {
        fprintf(stderr, "[texture] Floor.bmp not found — fallback to procedural\n");
    }
}
#endif 
