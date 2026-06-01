#ifndef OMPRENG_H
#define OMPRENG_H

#include "player.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ITEM_OMPRENG     10
#define MAX_OMPRENG      16
#define OMPRENG_PICKUP_R 1.65f
#define OMPRENG_BOB_SPEED 1.6f
#define OMPRENG_ROT_SPEED 45.0f
#define OMPRENG_LIFETIME  30.0f

#define SS_R       0.78f
#define SS_G       0.78f
#define SS_B       0.80f
#define SS_BRIGHT_R 0.92f
#define SS_BRIGHT_G 0.92f
#define SS_BRIGHT_B 0.95f
#define SS_DARK_R  0.55f
#define SS_DARK_G  0.55f
#define SS_DARK_B  0.58f

typedef struct {
    float x, y;
    float rotAngle;
    float bobPhase;
    float lifetime;
    int   active;
} Ompreng;

extern Ompreng gOmpreng[MAX_OMPRENG];
extern int     gNumOmpreng;

void omprengInit(void);
void omprengSpawn(float x, float y);
void omprengTryDrop(float x, float y, int enemyType);
void omprengApplyPickup(Player *player);
void omprengUpdate(Player *player, float dt);
void renderOmprengItems(Player *player);

#endif
