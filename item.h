#ifndef ITEM_H
#define ITEM_H

#include "player.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ITEM_HEALTH    0
#define ITEM_AMMO      1
#define ITEM_ARMOR     2
#define NUM_ITEM_TYPES 3
#define MAX_ITEMS      32
#define ITEM_RADIUS    1.65f
#define ITEM_BOB_SPEED 2.4f
#define ITEM_ROT_SPEED 90.0f
#define ITEM_LIFETIME  25.0f

typedef struct {
    int   type;
    float x, y;
    float rotAngle;
    float bobPhase;
    float lifetime;
    int   active;
} Item;

extern Item  gItems[MAX_ITEMS];
extern int   gNumItems;
extern int   gPlayerScore;
extern int   gWave;
extern float gWaveTimer;

int  itemGetScore(void);
int  itemGetWave(void);
void itemSpawn(int type, float x, float y);
void itemTryDrop(float x, float y);
void itemInit(void);
void itemUpdate(Player* player, float dt);
void renderItems(Player* player);

#endif
