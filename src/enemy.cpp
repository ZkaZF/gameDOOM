
#include "enemy.h"
#include "Ompreng.h"
#include "FoodItem.h"
#include "audio.h"
#include "weapon.h"
#include "raycaster.h"
#include "map.h"
#include "texture.h"
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

Enemy    gEnemies[MAX_ENEMIES];
int      gNumEnemies = 0;
int      gKillCount  = 0;
ArenaRoom gArenas[3];
float    gDamageFlash = 0.0f;

static int gWaveTop[3][3] = {
    {3, 0, 0}, {2, 0, 1}, {0, 1, 2},
};
static int gWaveBot[3][3] = {
    {2, 0, 1}, {3, 0, 1}, {2, 1, 0},
};
static int gWaveBoss[1][3] = {
    {0, 1, 0},   /* 1 wave: 1 Boss (reuse ENEMY_DEMON slot) */
};

void enemySpawn(int type, float x, float y) {
    Enemy* e;
    if (gNumEnemies >= MAX_ENEMIES) return;
    e = &gEnemies[gNumEnemies++];
    memset(e, 0, sizeof(Enemy));
    e->type = type; e->state = STATE_IDLE;
    e->x = x; e->y = y; e->active = 1; e->arenaId = -1;
    switch (type) {
        case ENEMY_IMP:     e->hp = e->maxHp = 60.0f;  e->speed = 0.048f; e->damage = 10.0f; e->atkRange = 2.40f; e->atkCooldown = 1.0f;  break;
        case ENEMY_DEMON:   e->hp = e->maxHp = 180.0f; e->speed = 0.027f; e->damage = 35.0f; e->atkRange = 2.70f; e->atkCooldown = 1.8f;  break;
        case ENEMY_SPECTRE: e->hp = e->maxHp = 35.0f;  e->speed = 0.078f; e->damage = 8.0f;  e->atkRange = 2.25f; e->atkCooldown = 0.65f; break;
    }
}

void enemySpawnArena(int type, float x, float y, int arenaId) {
    int slot = -1, i;
    for (i = 0; i < gNumEnemies; i++) {
        if (!gEnemies[i].active && gEnemies[i].state == STATE_DEAD) { slot = i; break; }
    }
    if (slot == -1) {
        if (gNumEnemies >= MAX_ENEMIES) return;
        slot = gNumEnemies++;
    }
    memset(&gEnemies[slot], 0, sizeof(Enemy));
    gEnemies[slot].active = 1; gEnemies[slot].arenaId = arenaId;
    gEnemies[slot].state  = STATE_CHASE;
    gEnemies[slot].x = x; gEnemies[slot].y = y;
    /* Set sprite type based on arena */
    gEnemies[slot].spriteType = (arenaId == 2) ? 1 : 0;
    /* Boss room (arenaId==2): override with boss stats */
    if (arenaId == 2) {
        gEnemies[slot].type        = ENEMY_DEMON;
        gEnemies[slot].hp          = gEnemies[slot].maxHp = 500.0f;
        gEnemies[slot].speed       = 0.035f;
        gEnemies[slot].damage      = 50.0f;
        gEnemies[slot].atkRange    = 3.0f;
        gEnemies[slot].atkCooldown = 1.5f;
        return;
    }
    switch (type) {
        case ENEMY_IMP:     gEnemies[slot].type = ENEMY_IMP;     gEnemies[slot].hp = gEnemies[slot].maxHp = 60.0f;  gEnemies[slot].speed = 0.048f; gEnemies[slot].damage = 10.0f; gEnemies[slot].atkRange = 2.40f; gEnemies[slot].atkCooldown = 1.0f;  break;
        case ENEMY_DEMON:   gEnemies[slot].type = ENEMY_DEMON;   gEnemies[slot].hp = gEnemies[slot].maxHp = 180.0f; gEnemies[slot].speed = 0.027f; gEnemies[slot].damage = 35.0f; gEnemies[slot].atkRange = 2.70f; gEnemies[slot].atkCooldown = 1.8f;  break;
        case ENEMY_SPECTRE: gEnemies[slot].type = ENEMY_SPECTRE; gEnemies[slot].hp = gEnemies[slot].maxHp = 35.0f;  gEnemies[slot].speed = 0.078f; gEnemies[slot].damage = 8.0f;  gEnemies[slot].atkRange = 2.25f; gEnemies[slot].atkCooldown = 0.65f; break;
    }
}

static void arenaLockDoor(ArenaRoom* a) {
    int i;
    for (i = 0; i < a->numDoorTiles; i++)
        worldMap[a->doorTiles[i][1]][a->doorTiles[i][0]] = 1;
}

static void arenaUnlockDoor(ArenaRoom* a) {
    int i;
    for (i = 0; i < a->numDoorTiles; i++)
        worldMap[a->doorTiles[i][1]][a->doorTiles[i][0]] = 0;
}

static void arenaSpawnWave(ArenaRoom* a) {
    int w = a->currentWave, i, si;
    int imp, dem, spc;
    int (*waveTable)[3];
    if (a->arenaId == 0) waveTable = gWaveTop;
    else if (a->arenaId == 1) waveTable = gWaveBot;
    else waveTable = gWaveBoss;
    if (w < 0 || w >= a->maxWaves) return;
    imp = waveTable[w][0]; dem = waveTable[w][1]; spc = waveTable[w][2];
    si = 0;
    for (i = 0; i < imp; i++) { int sp = si % a->numSpawns; enemySpawnArena(ENEMY_IMP,     a->spawnX[sp], a->spawnY[sp], a->arenaId); si++; }
    for (i = 0; i < dem; i++) { int sp = si % a->numSpawns; enemySpawnArena(ENEMY_DEMON,   a->spawnX[sp], a->spawnY[sp], a->arenaId); si++; }
    for (i = 0; i < spc; i++) { int sp = si % a->numSpawns; enemySpawnArena(ENEMY_SPECTRE, a->spawnX[sp], a->spawnY[sp], a->arenaId); si++; }
    sprintf(a->statusMsg, "WAVE %d / %d", w + 1, a->maxWaves);
    a->waveJustSpawned = 1;
}

static int arenaAliveCount(int arenaId) {
    int i, c = 0;
    for (i = 0; i < gNumEnemies; i++) {
        Enemy* e = &gEnemies[i];
        if (e->arenaId == arenaId && e->active && e->state != STATE_DEAD && e->state != STATE_DYING) c++;
    }
    return c;
}

static void arenaInit(void) {
    int i;
    /* --- Arena 0: Ruangan Atas (Top) --- */
    ArenaRoom* t = &gArenas[0];
    memset(t, 0, sizeof(ArenaRoom));
    t->arenaId = 0; t->state = ARENA_INACTIVE; t->currentWave = 0;
    t->maxWaves = ARENA_TOTAL_WAVES;
    t->minX = 27.0f; t->maxX = 66.0f; t->minY = 6.0f; t->maxY = 45.0f;
    t->numDoorTiles = 8;
    for (i = 0; i < 4; i++) {
        t->doorTiles[i*2][0] = 15;   t->doorTiles[i*2][1]   = 16 + i;
        t->doorTiles[i*2+1][0] = 16; t->doorTiles[i*2+1][1] = 16 + i;
    }
    t->numSpawns = 5;
    t->spawnX[0] = 33.0f; t->spawnY[0] = 13.5f;
    t->spawnX[1] = 60.0f; t->spawnY[1] = 13.5f;
    t->spawnX[2] = 33.0f; t->spawnY[2] = 37.5f;
    t->spawnX[3] = 60.0f; t->spawnY[3] = 37.5f;
    t->spawnX[4] = 46.5f; t->spawnY[4] = 25.5f;
    sprintf(t->statusMsg, "");
    /* --- Arena 1: Ruangan Bawah (Bottom) --- */
    ArenaRoom* b = &gArenas[1];
    memset(b, 0, sizeof(ArenaRoom));
    b->arenaId = 1; b->state = ARENA_INACTIVE; b->currentWave = 0;
    b->maxWaves = ARENA_TOTAL_WAVES;
    b->minX = 27.0f; b->maxX = 66.0f; b->minY = 96.0f; b->maxY = 135.0f;
    b->numDoorTiles = 6;
    for (i = 0; i < 3; i++) {
        b->doorTiles[i*2][0] = 15;   b->doorTiles[i*2][1]   = 28 + i;
        b->doorTiles[i*2+1][0] = 16; b->doorTiles[i*2+1][1] = 28 + i;
    }
    b->numSpawns = 5;
    b->spawnX[0] = 33.0f; b->spawnY[0] = 100.5f;
    b->spawnX[1] = 60.0f; b->spawnY[1] = 100.5f;
    b->spawnX[2] = 33.0f; b->spawnY[2] = 127.5f;
    b->spawnX[3] = 60.0f; b->spawnY[3] = 127.5f;
    b->spawnX[4] = 46.5f; b->spawnY[4] = 114.0f;
    sprintf(b->statusMsg, "");
    /* --- Arena 2: Ruangan Kanan (Boss Room) --- */
    ArenaRoom* r = &gArenas[2];
    memset(r, 0, sizeof(ArenaRoom));
    r->arenaId = 2; r->state = ARENA_INACTIVE; r->currentWave = 0;
    r->maxWaves = ARENA_BOSS_WAVES;   /* 1 wave saja */
    /* Ruangan kanan: logical cols 23-38, rows 16-35 -> world coords * MAP_SCALE(3) */
    r->minX = 72.0f; r->maxX = 114.0f; r->minY = 51.0f; r->maxY = 105.0f;
    /* Door tiles: lorong dari hub ke kanan (logical col 23, rows 27-29) */
    r->numDoorTiles = 6;
    for (i = 0; i < 3; i++) {
        r->doorTiles[i*2][0]   = 23; r->doorTiles[i*2][1]   = 27 + i;
        r->doorTiles[i*2+1][0] = 24; r->doorTiles[i*2+1][1] = 27 + i;
    }
    r->numSpawns = 2;
    r->spawnX[0] = 87.0f; r->spawnY[0] = 66.0f;   /* tengah ruangan kanan */
    r->spawnX[1] = 99.0f; r->spawnY[1] = 87.0f;
    sprintf(r->statusMsg, "");
}

static void arenaUpdate(Player* player, float dt) {
    int ai;
    for (ai = 0; ai < 3; ai++) {
        ArenaRoom* a = &gArenas[ai];
        /* Safety: once completed, never reactivate */
        if (a->completed && a->state != ARENA_COMPLETE) {
            a->state = ARENA_COMPLETE;
            a->statusTimer = 0.0f;
            sprintf(a->statusMsg, "");
        }
        switch (a->state) {
            case ARENA_INACTIVE:
                if (player->x >= a->minX && player->x <= a->maxX &&
                    player->y >= a->minY && player->y <= a->maxY) {
                    a->state = ARENA_ACTIVE; a->currentWave = 0;
                    arenaLockDoor(a); arenaSpawnWave(a);
                    audioPlayArena(ai);
                    printf("[Arena %d] Player masuk\n", ai);
                }
                break;
            case ARENA_ACTIVE: {
                int alive;
                if (a->waveJustSpawned) { a->waveJustSpawned = 0; break; }
                alive = arenaAliveCount(ai);
                if (alive == 0) {
                    if (a->currentWave >= a->maxWaves - 1) {
                        a->state = ARENA_COMPLETE; a->completed = 1;
                        arenaUnlockDoor(a);
                        audioStop();
                        sprintf(a->statusMsg, "ARENA CLEAR!"); a->statusTimer = 4.0f;
                        printf("[Arena %d] COMPLETED\n", ai);
                    } else {
                        a->state = ARENA_COOLDOWN; a->cooldownTimer = ARENA_COOLDOWN_TIME;
                        sprintf(a->statusMsg, "WAVE %d CLEAR!", a->currentWave + 1);
                    }
                } else {
                    sprintf(a->statusMsg, "WAVE %d / %d  [ %d left ]", a->currentWave+1, a->maxWaves, alive);
                }
                break;
            }
            case ARENA_COOLDOWN:
                a->cooldownTimer -= dt;
                sprintf(a->statusMsg, "NEXT WAVE IN %.0f...", a->cooldownTimer);
                if (a->cooldownTimer <= 0.0f) {
                    a->currentWave++; a->state = ARENA_ACTIVE; arenaSpawnWave(a);
                }
                break;
            case ARENA_COMPLETE:
                if (a->statusTimer > 0.0f) {
                    a->statusTimer -= dt;
                    if (a->statusTimer <= 0.0f) sprintf(a->statusMsg, "");
                }
                /* Stay in ARENA_COMPLETE forever — never go back to INACTIVE */
                break;
        }
    }
}

void enemyInitLevel(void) {
    memset(gEnemies, 0, sizeof(gEnemies));
    gNumEnemies = 0; gKillCount = 0; gDamageFlash = 0.0f;
    arenaUnlockDoor(&gArenas[0]);
    arenaUnlockDoor(&gArenas[1]);
    arenaUnlockDoor(&gArenas[2]);
    arenaInit();
}

static void enemyHit(Enemy* e, int damage) {
    if (e->state == STATE_DEAD || e->state == STATE_DYING) return;
    e->hp -= (float)damage;
    if (e->hp <= 0.0f) {
        e->hp = 0.0f; e->state = STATE_DYING; e->deathTimer = 0.0f;
        gKillCount++;
        /* Drop items based on arena */
        if (e->arenaId == 0) {
            /* Arena Atas: susu kotak / nasi putih */
            foodSpawn(rand() % 2 == 0 ? FOOD_SUSU_KOTAK : FOOD_NASI_PUTIH, e->x, e->y);
        } else if (e->arenaId == 1) {
            /* Arena Bawah: ayam goreng / telur ceplok / nasi putih */
            int types[] = {FOOD_AYAM_GORENG, FOOD_TELUR_CEPLOK, FOOD_NASI_PUTIH};
            foodSpawn(types[rand() % 3], e->x, e->y);
        } else if (e->arenaId == 2) {
            /* Boss Room: Ompreng only */
            omprengSpawn(e->x, e->y);
        }
    } else {
        if (e->state == STATE_IDLE) e->state = STATE_CHASE;
    }
}

static void enemyCheckProjectiles(void) {
    int i, j;
    for (j = 0; j < MAX_PROJECTILES; j++) {
        Projectile* p = &gProjectiles[j];
        if (!p->active) continue;
        for (i = 0; i < gNumEnemies; i++) {
            Enemy* e = &gEnemies[i];
            float dx, dy, dist;
            if (!e->active || e->state == STATE_DEAD || e->state == STATE_DYING) continue;
            dx = p->x - e->x; dy = p->y - e->y;
            dist = sqrtf(dx*dx + dy*dy);
            if (dist < 1.35f) { enemyHit(e, p->damage); p->active = 0; break; }
        }
    }
}

void enemyUpdate(Player* player, float dt) {
    int i;
    arenaUpdate(player, dt);
    enemyCheckProjectiles();
    for (i = 0; i < gNumEnemies; i++) {
        Enemy* e = &gEnemies[i];
        float  dx, dy, dist, nx, ny;
        if (!e->active) continue;
        dx = player->x - e->x; dy = player->y - e->y;
        dist = sqrtf(dx*dx + dy*dy);
        switch (e->state) {
            case STATE_IDLE:
                if (dist < DETECT_RANGE) e->state = STATE_CHASE;
                break;
            case STATE_CHASE:
                if (dist > DETECT_RANGE + 6.0f) {
                    e->stateTimer += dt;
                    if (e->stateTimer > 3.0f) { e->state = STATE_IDLE; e->stateTimer = 0.0f; }
                } else { e->stateTimer = 0.0f; }
                if (dist <= e->atkRange) { e->state = STATE_ATTACK; e->atkTimer = 0.0f; break; }
                if (dist > 0.01f) {
                    float spd = e->speed;
                    if (e->type == ENEMY_SPECTRE && dist < 7.5f) { nx = -(dx/dist)*spd*0.6f; ny = -(dy/dist)*spd*0.6f; }
                    else { nx = (dx/dist)*spd; ny = (dy/dist)*spd; }
                    if (isWalkable(e->x + nx + (nx > 0 ? 0.9f : -0.9f), e->y)) e->x += nx;
                    if (isWalkable(e->x, e->y + ny + (ny > 0 ? 0.9f : -0.9f))) e->y += ny;
                }
                break;
            case STATE_ATTACK:
                e->atkTimer += dt;
                if (e->atkTimer >= e->atkCooldown) {
                    e->atkTimer = 0.0f;
                    if (dist <= e->atkRange + 0.9f) {
                        player->health -= (int)e->damage;
                        if (player->health < 0) player->health = 0;
                        gDamageFlash = 1.0f;
                    } else { e->state = STATE_CHASE; }
                }
                if (dist > e->atkRange + 1.8f) e->state = STATE_CHASE;
                break;
            case STATE_DYING:
                e->deathTimer += dt;
                if (e->deathTimer >= DEATH_DURATION) { e->state = STATE_DEAD; e->active = 0; }
                break;
            case STATE_DEAD: e->active = 0; break;
        }
    }
}

static void renderImpModel(float death) {
    float sink = -death * 0.55f, tilt = death * 72.0f;
    glPushMatrix(); glTranslatef(0.0f, sink, 0.0f); glRotatef(-tilt, 1.0f, 0.0f, 0.0f);
    glColor3f(0.62f, 0.12f, 0.08f); glPushMatrix(); glScalef(0.36f, 0.42f, 0.26f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.72f, 0.19f, 0.10f); glPushMatrix(); glTranslatef(0.0f, 0.38f, 0.0f); glutSolidSphere(0.22f, 10, 8); glPopMatrix();
    glColor3f(0.28f, 0.08f, 0.04f);
    glPushMatrix(); glTranslatef(-0.10f, 0.56f, 0.0f); glRotatef(-20.0f, 0.0f, 0.0f, 1.0f); glutSolidCone(0.05f, 0.20f, 6, 1); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.10f, 0.56f, 0.0f); glRotatef( 20.0f, 0.0f, 0.0f, 1.0f); glutSolidCone(0.05f, 0.20f, 6, 1); glPopMatrix();
    glColor3f(1.0f, 0.85f, 0.0f);
    glPushMatrix(); glTranslatef(-0.09f, 0.41f, 0.18f); glutSolidSphere(0.05f, 6, 4); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.09f, 0.41f, 0.18f); glutSolidSphere(0.05f, 6, 4); glPopMatrix();
    glColor3f(0.55f, 0.11f, 0.07f);
    if (gQuad) {
        glPushMatrix(); glTranslatef(-0.22f, 0.08f, 0.0f); glRotatef(20.0f, 0.0f, 0.0f, 1.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.058f, 0.042f, 0.30f, 6, 1); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.22f, 0.08f, 0.0f); glRotatef(-20.0f, 0.0f, 0.0f, 1.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.058f, 0.042f, 0.30f, 6, 1); glPopMatrix();
        glColor3f(0.48f, 0.10f, 0.05f);
        glPushMatrix(); glTranslatef(-0.10f, -0.30f, 0.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.07f, 0.05f, 0.22f, 6, 1); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.10f, -0.30f, 0.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.07f, 0.05f, 0.22f, 6, 1); glPopMatrix();
    }
    glPopMatrix();
}

static void renderDemonModel(float death) {
    float sink = -death * 0.60f, tilt = death * 58.0f;
    glPushMatrix(); glTranslatef(0.0f, sink, 0.0f); glRotatef(-tilt, 1.0f, 0.0f, 0.0f);
    glColor3f(0.15f, 0.38f, 0.14f); glPushMatrix(); glScalef(0.56f, 0.58f, 0.42f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.20f, 0.45f, 0.17f); glPushMatrix(); glTranslatef(0.0f, 0.44f, 0.0f); glutSolidSphere(0.30f, 10, 8); glPopMatrix();
    glColor3f(0.95f, 0.12f, 0.05f);
    glPushMatrix(); glTranslatef(-0.12f, 0.48f, 0.26f); glutSolidSphere(0.07f, 6, 4); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.12f, 0.48f, 0.26f); glutSolidSphere(0.07f, 6, 4); glPopMatrix();
    glColor3f(0.17f, 0.40f, 0.14f);
    if (gQuad) {
        glPushMatrix(); glTranslatef(-0.40f, 0.10f, 0.0f); glRotatef(15.0f, 0.0f, 0.0f, 1.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.10f, 0.08f, 0.40f, 8, 1); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.40f, 0.10f, 0.0f); glRotatef(-15.0f, 0.0f, 0.0f, 1.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.10f, 0.08f, 0.40f, 8, 1); glPopMatrix();
        glColor3f(0.13f, 0.30f, 0.11f);
        glPushMatrix(); glTranslatef(-0.15f, -0.44f, 0.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.10f, 0.08f, 0.28f, 8, 1); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.15f, -0.44f, 0.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.10f, 0.08f, 0.28f, 8, 1); glPopMatrix();
    }
    glPopMatrix();
}

static void renderSpectreModel(float death) {
    float sink  = -death * 0.80f;
    float alpha = 0.68f * (1.0f - death * 0.92f);
    double t    = (double)glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    glPushMatrix(); glTranslatef(0.0f, sink + (float)(sin(t * 3.2) * 0.04f), 0.0f);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.10f, 0.52f, 0.82f, alpha); glPushMatrix(); glScalef(0.28f, 0.50f, 0.22f); glutSolidSphere(1.0f, 10, 8); glPopMatrix();
    glColor4f(0.45f, 0.88f, 1.0f, alpha*1.1f); glPushMatrix(); glScalef(0.14f, 0.26f, 0.12f); glutSolidSphere(1.0f, 8, 6); glPopMatrix();
    glColor4f(0.85f, 1.0f, 1.0f, 0.95f*(1.0f-death));
    glPushMatrix(); glTranslatef(-0.10f, 0.19f, 0.20f); glutSolidSphere(0.062f, 6, 4); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.10f, 0.19f, 0.20f); glutSolidSphere(0.062f, 6, 4); glPopMatrix();
    glColor4f(0.08f, 0.38f, 0.65f, alpha*0.75f);
    if (gQuad) {
        int k;
        for (k = 0; k < 3; k++) {
            float tx = (float)(k-1) * 0.12f;
            float tlen = 0.19f + (float)(sin(t * 2.1 + k * 2.0) * 0.04);
            glPushMatrix(); glTranslatef(tx, -0.32f, 0.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
                gluCylinder(gQuad, 0.030f, 0.008f, tlen, 5, 1); glPopMatrix();
        }
    }
    glDisable(GL_BLEND); glPopMatrix();
}
void renderEnemies(Player* player) {
    int   sortIdx[MAX_ENEMIES];
    float sortDist[MAX_ENEMIES];
    int   n = 0, si, sj;
    float det, aspect;
    det    = player->dirX * player->planeY - player->planeX * player->dirY;
    if (fabsf(det) < 0.00001f) det = 0.00001f;
    aspect = (float)SCREEN_W / (float)SCREEN_H;
    {
        int i;
        for (i = 0; i < gNumEnemies; i++) {
            Enemy* e = &gEnemies[i];
            float dx, dy;
            if (!e->active && e->state != STATE_DYING) continue;
            dx = e->x - player->x; dy = e->y - player->y;
            sortIdx[n] = i; sortDist[n] = dx*dx + dy*dy; n++;
        }
    }
    for (si = 1; si < n; si++) {
        int   idxTmp  = sortIdx[si];
        float distTmp = sortDist[si];
        sj = si - 1;
        while (sj >= 0 && sortDist[sj] < distTmp) {
            sortIdx[sj+1] = sortIdx[sj]; sortDist[sj+1] = sortDist[sj]; sj--;
        }
        sortIdx[sj+1] = idxTmp; sortDist[sj+1] = distTmp;
    }
    {
        int k;
        int pitchInt = (int)(player->pitch + player->jumpZ * 120.0f);
        int horizY   = SCREEN_H / 2 + pitchInt;
        for (k = 0; k < n; k++) {
            Enemy* e = &gEnemies[sortIdx[k]];
            float dx, dy, tX, tY, fullH, floorY, sScale, shade;
            int   screenX, behindWall, col, spriteH, spriteW, sX0, sX1, sY0, sY1;
            float fx0, fx1, fy0, fy1, midX, w, h;
            dx = e->x - player->x; dy = e->y - player->y;
            tX = (player->dirX * dy  - player->dirY * dx)  / det;
            tY = (player->planeY * dx - player->planeX * dy) / det;
            if (tY <= 0.1f) continue;
            screenX = (int)((float)(SCREEN_W / 2) * (1.0f + tX / tY));
            /* Boss (spriteType==1) lebih besar dari Goblin biasa */
            if (e->spriteType == 1)
                sScale = 1.4f;
            else
                sScale = (e->type == ENEMY_DEMON) ? 0.90f : (e->type == ENEMY_SPECTRE) ? 0.70f : 0.80f;
            if (e->state == STATE_DYING) {
                float t2 = e->deathTimer / DEATH_DURATION;
                if (t2 > 1.0f) t2 = 1.0f;
                sScale *= (1.0f - t2 * 0.85f);
            }
            fullH   = (float)SCREEN_H * WALL_HEIGHT_SCALE / tY;
            floorY  = (float)horizY + fullH * 0.5f;
            spriteH = (int)(fullH * sScale);
            if (spriteH < 2) spriteH = 2;
            spriteW = spriteH;
            sY1 = (int)floorY; sY0 = sY1 - spriteH;
            sX0 = screenX - spriteW/2; sX1 = screenX + spriteW/2;
            if (sX1 < 0 || sX0 >= SCREEN_W || sY1 < 0 || sY0 >= SCREEN_H) continue;
            behindWall = 1;
            { int step = (spriteW > 16) ? spriteW/6 : 1;
              for (col = sX0; col <= sX1; col += step)
                  if (col >= 0 && col < SCREEN_W && zBuffer[col] >= tY * 1.02f) { behindWall = 0; break; } }
            if (behindWall) continue;
            shade = 1.0f - tY / 48.0f;
            if (shade < 0.15f) shade = 0.15f;
            fx0 = (float)(sX0 < 0 ? 0 : sX0);
            fx1 = (float)(sX1 >= SCREEN_W ? SCREEN_W-1 : sX1);
            fy0 = (float)(sY0 < 0 ? 0 : sY0);
            fy1 = (float)(sY1 >= SCREEN_H ? SCREEN_H-1 : sY1);
            midX = (fx0 + fx1) * 0.5f;
            w = fx1 - fx0; h = fy1 - fy0;
            if (w < 1 || h < 1) continue;
            {
                GLuint chosenTex = 0;
                if (e->spriteType == 1 && gBossSpriteGLTex != 0)
                    chosenTex = gBossSpriteGLTex;
                else if (e->spriteType == 0 && gGoblinSpriteGLTex != 0)
                    chosenTex = gGoblinSpriteGLTex;
                if (chosenTex != 0) {
                    float alpha = 1.0f;
                    if (e->state == STATE_DYING) {
                        float t2 = e->deathTimer / DEATH_DURATION;
                        if (t2 > 1.0f) t2 = 1.0f;
                        alpha = 1.0f - t2;
                    }
                    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glEnable(GL_COLOR_MATERIAL);
                    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, chosenTex);
                    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
                    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glColor4f(1.0f, 1.0f, 1.0f, alpha);
                    glBegin(GL_QUADS);
                        glTexCoord2f(0.0f, 0.0f); glVertex2f(fx0, fy0);
                        glTexCoord2f(1.0f, 0.0f); glVertex2f(fx1, fy0);
                        glTexCoord2f(1.0f, 1.0f); glVertex2f(fx1, fy1);
                        glTexCoord2f(0.0f, 1.0f); glVertex2f(fx0, fy1);
                    glEnd();
                    glDisable(GL_BLEND); glDisable(GL_TEXTURE_2D); glDisable(GL_COLOR_MATERIAL);
                    glBindTexture(GL_TEXTURE_2D, 0); glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                } else {
                    /* Fallback: flat color rectangle */
                    glColor3f(shade * 0.45f, shade * 0.12f, shade * 0.08f);
                    glBegin(GL_QUADS); glVertex2f(fx0,fy0); glVertex2f(fx1,fy0); glVertex2f(fx1,fy1); glVertex2f(fx0,fy1); glEnd();
                    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                }
            }
            if (e->state != STATE_DYING && e->state != STATE_DEAD && tY < 21.0f) {
                float hr   = e->hp / e->maxHp;
                float barY = fy0 - (h > 40 ? h*0.08f : 4.0f);
                float barH = (h > 40 ? h*0.05f : 3.0f);
                glColor3f(0.15f,0.0f,0.0f);
                glBegin(GL_QUADS); glVertex2f(fx0,barY); glVertex2f(fx1,barY); glVertex2f(fx1,barY+barH); glVertex2f(fx0,barY+barH); glEnd();
                if (hr > 0.5f) glColor3f(0.10f,0.80f,0.20f);
                else if (hr > 0.25f) glColor3f(0.90f,0.70f,0.10f);
                else glColor3f(0.90f,0.10f,0.08f);
                glBegin(GL_QUADS); glVertex2f(fx0,barY); glVertex2f(fx0+w*hr,barY); glVertex2f(fx0+w*hr,barY+barH); glVertex2f(fx0,barY+barH); glEnd();
            }
        }
    }
}

int enemyGetKillCount(void)  { return gKillCount; }
int enemyGetAliveCount(void) {
    int i, c = 0;
    for (i = 0; i < gNumEnemies; i++) if (gEnemies[i].active) c++;
    return c;
}
