#ifndef ENEMY_H
#define ENEMY_H

#include "player.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ENEMY_IMP       0
#define ENEMY_DEMON     1
#define ENEMY_SPECTRE   2
#define NUM_ENEMY_TYPES 3
#define STATE_IDLE      0
#define STATE_CHASE     1
#define STATE_ATTACK    2
#define STATE_DYING     3
#define STATE_DEAD      4
#define MAX_ENEMIES     30
#define DETECT_RANGE    24.0f
#define DEATH_DURATION  1.3f
#define ARENA_INACTIVE  0
#define ARENA_ACTIVE    1
#define ARENA_COOLDOWN  2
#define ARENA_COMPLETE  3
#define ARENA_TOTAL_WAVES 5
#define ARENA_COOLDOWN_TIME 2.5f

typedef struct {
    int   type;
    int   state;
    float x, y;
    float hp, maxHp;
    float speed;
    float damage;
    float atkRange;
    float atkCooldown;
    float atkTimer;
    float stateTimer;
    float deathTimer;
    int   active;
    int   arenaId;
} Enemy;

typedef struct {
    int   state;
    int   currentWave;
    float cooldownTimer;
    int   waveJustSpawned;
    int   completed;
    float minX, maxX, minY, maxY;
    int   doorTiles[8][2];
    int   numDoorTiles;
    float spawnX[5];
    float spawnY[5];
    int   numSpawns;
    int   arenaId;
    char  statusMsg[48];
    float statusTimer;
} ArenaRoom;

extern Enemy     gEnemies[MAX_ENEMIES];
extern int       gNumEnemies;
extern int       gKillCount;
extern ArenaRoom gArenas[2];
extern float     gDamageFlash;

void enemySpawn(int type, float x, float y);
void enemySpawnArena(int type, float x, float y, int arenaId);
void enemyInitLevel(void);
void enemyUpdate(Player* player, float dt);
void renderEnemies(Player* player);
int  enemyGetKillCount(void);
int  enemyGetAliveCount(void);

#endif
