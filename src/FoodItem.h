#ifndef FOODITEM_H
#define FOODITEM_H

#include "player.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FOOD_SUSU_KOTAK    0
#define FOOD_NASI_PUTIH    1
#define FOOD_AYAM_GORENG   2
#define FOOD_TELUR_CEPLOK  3
#define FOOD_TYPE_COUNT    4

#define MAX_FOOD_ITEMS     32
#define FOOD_PICKUP_R      1.65f
#define FOOD_BOB_SPEED     1.6f
#define FOOD_LIFETIME      30.0f

typedef struct {
    float x, y;
    int   type;
    float bobPhase;
    float lifetime;
    int   active;
} FoodItem;

extern FoodItem gFoodItems[MAX_FOOD_ITEMS];
extern int      gNumFoodItems;
extern int      gFoodCollected[FOOD_TYPE_COUNT];
extern int      gOmprengCollected;

void foodInit(void);
void foodSpawn(int type, float x, float y);
void foodUpdate(Player *player, float dt);
void renderFoodItems(Player *player);
int  foodAllCollected(void);

#endif
