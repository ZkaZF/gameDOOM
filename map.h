#ifndef MAP_DATA_H
#define MAP_DATA_H

#define MAP_SCALE     3
#define MAP_LOGICAL_W 40
#define MAP_LOGICAL_H 48
#define MAP_W (MAP_LOGICAL_W * MAP_SCALE)
#define MAP_H (MAP_LOGICAL_H * MAP_SCALE)
#define PLAYER_START_X  13.5f
#define PLAYER_START_Y  70.5f
#define PLAYER_START_ANGLE 0.0f

extern int worldMap[MAP_LOGICAL_H][MAP_LOGICAL_W];

int getMap(int x, int y);
int isWalkable(float x, float y);

#endif
