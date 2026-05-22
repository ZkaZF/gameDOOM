#ifndef TEXTURE_H
#define TEXTURE_H

#include <GL/glut.h>

typedef struct {
    unsigned char *pixels;
    int width;
    int height;
} BmpTexture;

extern BmpTexture gFloorTex;
extern BmpTexture gWallTex;
extern BmpTexture gEnemySpriteTex;
extern GLuint     gEnemySpriteGLTex;

float clampf(float v, float lo, float hi);
float lerpf(float a, float b, float t);
void  applyFog(float* r, float* g, float* b, float distance, float maxDist);
int   loadBMP(const char *filename, BmpTexture *tex);
void  sampleBmpTile(const BmpTexture *tex, float wx, float wy, float tileScale,
                    float *r, float *g, float *b);
void  initFloorTexture(void);
void  initWallTexture(void);
void  initEnemySpriteTexture(void);
void  sampleWallBilinear(const BmpTexture *tex, float u, float v,
                          float *r, float *g, float *b);

#endif
