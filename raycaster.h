#ifndef RAYCASTER_H
#define RAYCASTER_H

#include "player.h"

#define SCREEN_W 800
#define SCREEN_H 600
#define WALL_HEIGHT_SCALE 3.5f
#define WALL_TEX_TILE_SCALE 3.0f

extern float wallColors[][3];
extern int   numWallColors;
extern float zBuffer[SCREEN_W];

void renderRaycastView(Player *player);

#endif
