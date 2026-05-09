#ifndef ENEMY_H
#define ENEMY_H

/*
 * enemy.h — Enemy System (V3)
 *
 * 3 Enemy Types:
 *   ENEMY_IMP     — Basic red demon, medium HP/speed
 *   ENEMY_DEMON   — Green tank, high HP/damage, slow
 *   ENEMY_SPECTRE — Fast blue ghost, low HP, hit & run
 *
 * AI State Machine per enemy:
 *   IDLE → CHASE → ATTACK → (back to CHASE)
 *   Any  → DYING → DEAD
 *
 * Rendering: true 3D GLUT models via single gluPerspective pass
 *   matching the raycaster camera. Z-buffer test against walls
 *   via zBuffer[] array from raycaster.h.
 *
 * Depends (included before in main.cpp):
 *   map.h, player.h, raycaster.h, weapon.h
 */

#include <GL/glut.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ───────────────────── Constants ───────────────────── */
#define ENEMY_IMP      0
#define ENEMY_DEMON    1
#define ENEMY_SPECTRE  2
#define NUM_ENEMY_TYPES 3

#define STATE_IDLE    0
#define STATE_CHASE   1
#define STATE_ATTACK  2
#define STATE_DYING   3
#define STATE_DEAD    4

#define MAX_ENEMIES    20
#define DETECT_RANGE   24.0f   /* 8.0f × MAP_SCALE(3) */
#define DEATH_DURATION 1.3f

/* ───────────────────── Struct ───────────────────── */
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
} Enemy;

/* ───────────────────── Globals ───────────────────── */
static Enemy gEnemies[MAX_ENEMIES];
static int   gNumEnemies = 0;
static int   gKillCount  = 0;

/* Damage flash (set when player takes damage, fades in hud) */
static float gDamageFlash = 0.0f;

/* ───────────────────── Spawn ───────────────────── */
static void enemySpawn(int type, float x, float y) {
    Enemy* e;
    if (gNumEnemies >= MAX_ENEMIES) return;
    e = &gEnemies[gNumEnemies++];

    memset(e, 0, sizeof(Enemy));
    e->type   = type;
    e->state  = STATE_IDLE;
    e->x      = x;
    e->y      = y;
    e->active = 1;

    switch (type) {
        case ENEMY_IMP:
            e->hp          = 60.0f;  e->maxHp = 60.0f;
            e->speed       = 0.048f;  /* 0.016f × 3 */
            e->damage      = 10.0f;
            e->atkRange    = 2.40f;   /* 0.80f × 3 */
            e->atkCooldown = 1.0f;
            break;
        case ENEMY_DEMON:
            e->hp          = 180.0f; e->maxHp = 180.0f;
            e->speed       = 0.027f;  /* 0.009f × 3 */
            e->damage      = 35.0f;
            e->atkRange    = 2.70f;   /* 0.90f × 3 */
            e->atkCooldown = 1.8f;
            break;
        case ENEMY_SPECTRE:
            e->hp          = 35.0f;  e->maxHp = 35.0f;
            e->speed       = 0.078f;  /* 0.026f × 3 */
            e->damage      = 8.0f;
            e->atkRange    = 2.25f;   /* 0.75f × 3 */
            e->atkCooldown = 0.65f;
            break;
    }
}

/* ───────────────────── Level Init ───────────────────── */
static void enemyInitLevel(void) {
    memset(gEnemies, 0, sizeof(gEnemies));
    gNumEnemies = 0;
    gKillCount  = 0;
    gDamageFlash = 0.0f;

    /* 5 Imps — tersebar di berbagai ruangan (posisi × MAP_SCALE = 3) */
    enemySpawn(ENEMY_IMP,    13.5f, 46.5f);   /* left room   — tengah */
    enemySpawn(ENEMY_IMP,    46.5f, 13.5f);   /* top room    — tengah */
    enemySpawn(ENEMY_IMP,    93.0f, 33.0f);   /* right room  — pojok atas */
    enemySpawn(ENEMY_IMP,    93.0f, 60.0f);   /* right room  — pojok bawah */
    enemySpawn(ENEMY_IMP,    46.5f, 79.5f);   /* bottom room — tengah */

    /* 2 Demons — menjaga hub/center */
    enemySpawn(ENEMY_DEMON,  43.5f, 46.5f);   /* hub — sisi kiri */
    enemySpawn(ENEMY_DEMON,  52.5f, 46.5f);   /* hub — sisi kanan */

    /* 3 Spectres — di koridor + right room besar */
    enemySpawn(ENEMY_SPECTRE, 46.5f, 31.5f);  /* koridor atas */
    enemySpawn(ENEMY_SPECTRE, 46.5f, 64.5f);  /* koridor bawah */
    enemySpawn(ENEMY_SPECTRE, 31.5f, 46.5f);  /* koridor kiri */
}

/* ───────────────────── Hit ───────────────────── */
static void enemyHit(Enemy* e, int damage) {
    if (e->state == STATE_DEAD || e->state == STATE_DYING) return;
    e->hp -= (float)damage;
    if (e->hp <= 0.0f) {
        e->hp        = 0.0f;
        e->state     = STATE_DYING;
        e->deathTimer = 0.0f;
        gKillCount++;
    } else {
        if (e->state == STATE_IDLE) e->state = STATE_CHASE;
    }
}

/* ───────────────────── Projectile ↔ Enemy Collision ───────────────────── */
static void enemyCheckProjectiles(void) {
    int i, j;
    for (j = 0; j < MAX_PROJECTILES; j++) {
        Projectile* p = &gProjectiles[j];
        if (!p->active) continue;
        for (i = 0; i < gNumEnemies; i++) {
            Enemy* e = &gEnemies[i];
            float dx, dy, dist;
            if (!e->active || e->state == STATE_DEAD || e->state == STATE_DYING) continue;
            dx   = p->x - e->x;
            dy   = p->y - e->y;
            dist = sqrtf(dx * dx + dy * dy);
            if (dist < 0.45f) {
                enemyHit(e, p->damage);
                p->active = 0;
                break;
            }
        }
    }
}

/* ───────────────────── AI Update ───────────────────── */
static void enemyUpdate(Player* player, float dt) {
    int i;
    enemyCheckProjectiles();

    for (i = 0; i < gNumEnemies; i++) {
        Enemy* e = &gEnemies[i];
        float  dx, dy, dist, nx, ny;

        if (!e->active) continue;

        dx   = player->x - e->x;
        dy   = player->y - e->y;
        dist = sqrtf(dx * dx + dy * dy);

        switch (e->state) {

            case STATE_IDLE:
                if (dist < DETECT_RANGE)
                    e->state = STATE_CHASE;
                break;

            case STATE_CHASE:
                /* Lose interest if player is far for 3s */
                if (dist > DETECT_RANGE + 6.0f) {  /* +2.0f × 3 */
                    e->stateTimer += dt;
                    if (e->stateTimer > 3.0f) {
                        e->state = STATE_IDLE;
                        e->stateTimer = 0.0f;
                    }
                } else {
                    e->stateTimer = 0.0f;
                }

                /* Switch to attack if in range */
                if (dist <= e->atkRange) {
                    e->state    = STATE_ATTACK;
                    e->atkTimer = 0.0f;
                    break;
                }

                /* Move toward player */
                if (dist > 0.01f) {
                    float spd = e->speed;
                    /* Spectre: retreat when too close */
                    if (e->type == ENEMY_SPECTRE && dist < 7.5f) {  /* 2.5f × 3 */
                        nx = -(dx / dist) * spd * 0.6f;
                        ny = -(dy / dist) * spd * 0.6f;
                    } else {
                        nx = (dx / dist) * spd;
                        ny = (dy / dist) * spd;
                    }
                    /* Slide-style collision */
                    if (isWalkable(e->x + nx + (nx > 0 ? 0.9f : -0.9f), e->y))
                        e->x += nx;
                    if (isWalkable(e->x, e->y + ny + (ny > 0 ? 0.9f : -0.9f)))
                        e->y += ny;
                }
                break;

            case STATE_ATTACK:
                e->atkTimer += dt;
                if (e->atkTimer >= e->atkCooldown) {
                    e->atkTimer = 0.0f;
                    if (dist <= e->atkRange + 0.9f) {  /* 0.3f × 3 */
                        /* Deal damage */
                        player->health -= (int)e->damage;
                        if (player->health < 0) player->health = 0;
                        gDamageFlash = 1.0f; /* red screen flash */
                    } else {
                        e->state = STATE_CHASE;
                    }
                }
                /* Chase again if target moves away */
                if (dist > e->atkRange + 1.8f)  /* 0.6f × 3 */
                    e->state = STATE_CHASE;
                break;

            case STATE_DYING:
                e->deathTimer += dt;
                if (e->deathTimer >= DEATH_DURATION) {
                    e->state  = STATE_DEAD;
                    e->active = 0;
                }
                break;

            case STATE_DEAD:
                e->active = 0;
                break;
        }
    }
}

/* ═══════════════════ 3D MODEL RENDERERS ═══════════════════ */

/* ── Imp (Red Demon) ── */
static void renderImpModel(float death) {
    float sink = -death * 0.55f;
    float tilt = death * 72.0f;

    glPushMatrix();
    glTranslatef(0.0f, sink, 0.0f);
    glRotatef(-tilt, 1.0f, 0.0f, 0.0f);

    /* Body */
    glColor3f(0.62f, 0.12f, 0.08f);
    glPushMatrix(); glScalef(0.36f, 0.42f, 0.26f); glutSolidCube(1.0f); glPopMatrix();

    /* Head */
    glColor3f(0.72f, 0.19f, 0.10f);
    glPushMatrix(); glTranslatef(0.0f, 0.38f, 0.0f); glutSolidSphere(0.22f, 10, 8); glPopMatrix();

    /* Horns */
    glColor3f(0.28f, 0.08f, 0.04f);
    glPushMatrix(); glTranslatef(-0.10f, 0.56f, 0.0f); glRotatef(-20.0f, 0.0f, 0.0f, 1.0f); glutSolidCone(0.05f, 0.20f, 6, 1); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.10f, 0.56f, 0.0f); glRotatef( 20.0f, 0.0f, 0.0f, 1.0f); glutSolidCone(0.05f, 0.20f, 6, 1); glPopMatrix();

    /* Eyes (yellow glow) */
    glColor3f(1.0f, 0.85f, 0.0f);
    glPushMatrix(); glTranslatef(-0.09f, 0.41f, 0.18f); glutSolidSphere(0.05f, 6, 4); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.09f, 0.41f, 0.18f); glutSolidSphere(0.05f, 6, 4); glPopMatrix();

    /* Arms */
    glColor3f(0.55f, 0.11f, 0.07f);
    if (gQuad) {
        glPushMatrix(); glTranslatef(-0.22f, 0.08f, 0.0f); glRotatef(20.0f, 0.0f, 0.0f, 1.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.058f, 0.042f, 0.30f, 6, 1); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.22f, 0.08f, 0.0f); glRotatef(-20.0f, 0.0f, 0.0f, 1.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.058f, 0.042f, 0.30f, 6, 1); glPopMatrix();
        /* Legs */
        glColor3f(0.48f, 0.10f, 0.05f);
        glPushMatrix(); glTranslatef(-0.10f, -0.30f, 0.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.07f, 0.05f, 0.22f, 6, 1); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.10f, -0.30f, 0.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.07f, 0.05f, 0.22f, 6, 1); glPopMatrix();
    }
    glPopMatrix();
}

/* ── Demon (Green Tank) ── */
static void renderDemonModel(float death) {
    float sink = -death * 0.60f;
    float tilt = death * 58.0f;

    glPushMatrix();
    glTranslatef(0.0f, sink, 0.0f);
    glRotatef(-tilt, 1.0f, 0.0f, 0.0f);

    /* Massive body */
    glColor3f(0.15f, 0.38f, 0.14f);
    glPushMatrix(); glScalef(0.56f, 0.58f, 0.42f); glutSolidCube(1.0f); glPopMatrix();

    /* Large head */
    glColor3f(0.20f, 0.45f, 0.17f);
    glPushMatrix(); glTranslatef(0.0f, 0.44f, 0.0f); glutSolidSphere(0.30f, 10, 8); glPopMatrix();

    /* Red eyes */
    glColor3f(0.95f, 0.12f, 0.05f);
    glPushMatrix(); glTranslatef(-0.12f, 0.48f, 0.26f); glutSolidSphere(0.07f, 6, 4); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.12f, 0.48f, 0.26f); glutSolidSphere(0.07f, 6, 4); glPopMatrix();

    /* Thick arms */
    glColor3f(0.17f, 0.40f, 0.14f);
    if (gQuad) {
        glPushMatrix(); glTranslatef(-0.40f, 0.10f, 0.0f); glRotatef(15.0f, 0.0f, 0.0f, 1.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.10f, 0.08f, 0.40f, 8, 1); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.40f, 0.10f, 0.0f); glRotatef(-15.0f, 0.0f, 0.0f, 1.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.10f, 0.08f, 0.40f, 8, 1); glPopMatrix();
        /* Legs */
        glColor3f(0.13f, 0.30f, 0.11f);
        glPushMatrix(); glTranslatef(-0.15f, -0.44f, 0.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.10f, 0.08f, 0.28f, 8, 1); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.15f, -0.44f, 0.0f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f); gluCylinder(gQuad, 0.10f, 0.08f, 0.28f, 8, 1); glPopMatrix();
    }
    glPopMatrix();
}

/* ── Spectre (Blue Ghost) ── */
static void renderSpectreModel(float death) {
    float sink  = -death * 0.80f;
    float alpha = 0.68f * (1.0f - death * 0.92f);
    double t    = (double)glutGet(GLUT_ELAPSED_TIME) / 1000.0;

    glPushMatrix();
    glTranslatef(0.0f, sink + (float)(sin(t * 3.2) * 0.04f), 0.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Main body */
    glColor4f(0.10f, 0.52f, 0.82f, alpha);
    glPushMatrix(); glScalef(0.28f, 0.50f, 0.22f); glutSolidSphere(1.0f, 10, 8); glPopMatrix();

    /* Glowing core */
    glColor4f(0.45f, 0.88f, 1.0f, alpha * 1.1f);
    glPushMatrix(); glScalef(0.14f, 0.26f, 0.12f); glutSolidSphere(1.0f, 8, 6); glPopMatrix();

    /* Eyes */
    glColor4f(0.85f, 1.0f, 1.0f, 0.95f * (1.0f - death));
    glPushMatrix(); glTranslatef(-0.10f, 0.19f, 0.20f); glutSolidSphere(0.062f, 6, 4); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.10f, 0.19f, 0.20f); glutSolidSphere(0.062f, 6, 4); glPopMatrix();

    /* Floating tendrils */
    glColor4f(0.08f, 0.38f, 0.65f, alpha * 0.75f);
    if (gQuad) {
        int k;
        for (k = 0; k < 3; k++) {
            float tx   = (float)(k - 1) * 0.12f;
            float tlen = 0.19f + (float)(sin(t * 2.1 + k * 2.0) * 0.04);
            glPushMatrix();
                glTranslatef(tx, -0.32f, 0.0f);
                glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
                gluCylinder(gQuad, 0.030f, 0.008f, tlen, 5, 1);
            glPopMatrix();
        }
    }
    glDisable(GL_BLEND);
    glPopMatrix();
}

/* ═══════════════════ RENDER PASS ═══════════════════ */
static void renderEnemies(Player* player) {
    /* Sort indices farthest-first for correct transparency blending */
    int   sortIdx[MAX_ENEMIES];
    float sortDist[MAX_ENEMIES];
    int   n = 0, si, sj;
    float det, aspect;

    det = player->dirX * player->planeY - player->planeX * player->dirY;
    if (fabsf(det) < 0.00001f) det = 0.00001f;
    aspect = (float)SCREEN_W / (float)SCREEN_H;

    /* Build list of active (non-dead) enemies */
    {
        int i;
        for (i = 0; i < gNumEnemies; i++) {
            Enemy* e = &gEnemies[i];
            float dx, dy;
            if (!e->active && e->state != STATE_DYING) continue;
            dx = e->x - player->x;
            dy = e->y - player->y;
            sortIdx[n]    = i;
            sortDist[n]   = dx * dx + dy * dy;
            n++;
        }
    }

    /* Insertion sort (farthest first) */
    for (si = 1; si < n; si++) {
        int   idxTmp  = sortIdx[si];
        float distTmp = sortDist[si];
        sj = si - 1;
        while (sj >= 0 && sortDist[sj] < distTmp) {
            sortIdx[sj + 1]  = sortIdx[sj];
            sortDist[sj + 1] = sortDist[sj];
            sj--;
        }
        sortIdx[sj + 1]  = idxTmp;
        sortDist[sj + 1] = distTmp;
    }

    /* ── 2D Billboard sprites — pure ortho, synced with WALL_HEIGHT_SCALE ── */
    {
        int k;
        int pitchInt = (int)player->pitch;
        int horizY   = SCREEN_H / 2 + pitchInt;

        for (k = 0; k < n; k++) {
            Enemy* e = &gEnemies[sortIdx[k]];
            float dx, dy, tX, tY;
            int   screenX, behindWall, col;
            float fullH, floorY, sScale, shade;
            int   spriteH, spriteW, sX0, sX1, sY0, sY1;
            float fx0, fx1, fy0, fy1, midX, w, h;

            dx = e->x - player->x;
            dy = e->y - player->y;

            tX = (player->dirX * dy  - player->dirY * dx)  / det;
            tY = (player->planeY * dx - player->planeX * dy) / det;
            if (tY <= 0.1f) continue;

            screenX = (int)((float)(SCREEN_W / 2) * (1.0f + tX / tY));

            /* Sprite scale by type, shrink on death */
            sScale = (e->type == ENEMY_DEMON) ? 0.90f :
                     (e->type == ENEMY_SPECTRE) ? 0.70f : 0.80f;
            if (e->state == STATE_DYING) {
                float t = e->deathTimer / DEATH_DURATION;
                if (t > 1.0f) t = 1.0f;
                sScale *= (1.0f - t * 0.85f);
            }

            /* Sprite dimensions — sits on floor */
            fullH   = (float)SCREEN_H * WALL_HEIGHT_SCALE / tY;
            floorY  = (float)horizY + fullH * 0.5f;
            spriteH = (int)(fullH * sScale);
            if (spriteH < 2) spriteH = 2;
            spriteW = spriteH;
            sY1 = (int)floorY;
            sY0 = sY1 - spriteH;
            sX0 = screenX - spriteW / 2;
            sX1 = screenX + spriteW / 2;
            if (sX1 < 0 || sX0 >= SCREEN_W || sY1 < 0 || sY0 >= SCREEN_H) continue;

            /* Wall occlusion */
            behindWall = 1;
            { int step = (spriteW > 16) ? spriteW / 6 : 1;
              for (col = sX0; col <= sX1; col += step)
                  if (col >= 0 && col < SCREEN_W && zBuffer[col] >= tY * 0.90f)
                      { behindWall = 0; break; } }
            if (behindWall) continue;

            shade = 1.0f - tY / 48.0f;
            if (shade < 0.15f) shade = 0.15f;

            /* Clamp draw rect */
            fx0  = (float)(sX0 < 0 ? 0 : sX0);
            fx1  = (float)(sX1 >= SCREEN_W ? SCREEN_W-1 : sX1);
            fy0  = (float)(sY0 < 0 ? 0 : sY0);
            fy1  = (float)(sY1 >= SCREEN_H ? SCREEN_H-1 : sY1);
            midX = (fx0 + fx1) * 0.5f;
            w    = fx1 - fx0;
            h    = fy1 - fy0;
            if (w < 1 || h < 1) continue;

            if (e->type == ENEMY_IMP) {
                /* Body: red-brown */
                glColor3f(0.45f*shade, 0.12f*shade, 0.08f*shade);
                glBegin(GL_QUADS); glVertex2f(fx0,fy0+h*0.28f); glVertex2f(fx1,fy0+h*0.28f); glVertex2f(fx1,fy1); glVertex2f(fx0,fy1); glEnd();
                /* Head: orange */
                glColor3f(0.72f*shade, 0.35f*shade, 0.10f*shade);
                glBegin(GL_QUADS); glVertex2f(midX-w*0.26f,fy0); glVertex2f(midX+w*0.26f,fy0); glVertex2f(midX+w*0.26f,fy0+h*0.28f); glVertex2f(midX-w*0.26f,fy0+h*0.28f); glEnd();
                /* Eyes: yellow */
                glColor3f(0.95f*shade, 0.85f*shade, 0.0f);
                glBegin(GL_QUADS); glVertex2f(midX-w*0.20f,fy0+h*0.07f); glVertex2f(midX-w*0.06f,fy0+h*0.07f); glVertex2f(midX-w*0.06f,fy0+h*0.18f); glVertex2f(midX-w*0.20f,fy0+h*0.18f); glEnd();
                glBegin(GL_QUADS); glVertex2f(midX+w*0.06f,fy0+h*0.07f); glVertex2f(midX+w*0.20f,fy0+h*0.07f); glVertex2f(midX+w*0.20f,fy0+h*0.18f); glVertex2f(midX+w*0.06f,fy0+h*0.18f); glEnd();
            } else if (e->type == ENEMY_DEMON) {
                /* Body: dark purple */
                glColor3f(0.35f*shade, 0.08f*shade, 0.30f*shade);
                glBegin(GL_QUADS); glVertex2f(fx0,fy0); glVertex2f(fx1,fy0); glVertex2f(fx1,fy1); glVertex2f(fx0,fy1); glEnd();
                /* Face: pink */
                glColor3f(0.70f*shade, 0.25f*shade, 0.30f*shade);
                glBegin(GL_QUADS); glVertex2f(midX-w*0.28f,fy0+h*0.08f); glVertex2f(midX+w*0.28f,fy0+h*0.08f); glVertex2f(midX+w*0.28f,fy0+h*0.42f); glVertex2f(midX-w*0.28f,fy0+h*0.42f); glEnd();
                /* Red eyes */
                glColor3f(0.95f*shade, 0.05f*shade, 0.05f*shade);
                glBegin(GL_QUADS); glVertex2f(midX-w*0.22f,fy0+h*0.12f); glVertex2f(midX-w*0.08f,fy0+h*0.12f); glVertex2f(midX-w*0.08f,fy0+h*0.24f); glVertex2f(midX-w*0.22f,fy0+h*0.24f); glEnd();
                glBegin(GL_QUADS); glVertex2f(midX+w*0.08f,fy0+h*0.12f); glVertex2f(midX+w*0.22f,fy0+h*0.12f); glVertex2f(midX+w*0.22f,fy0+h*0.24f); glVertex2f(midX+w*0.08f,fy0+h*0.24f); glEnd();
            } else { /* SPECTRE */
                /* Ghost body: blue-gray */
                glColor3f(0.50f*shade, 0.55f*shade, 0.62f*shade);
                glBegin(GL_QUADS); glVertex2f(fx0,fy0); glVertex2f(fx1,fy0); glVertex2f(fx1,fy1); glVertex2f(fx0,fy1); glEnd();
                /* Inner dark */
                glColor3f(0.25f*shade, 0.27f*shade, 0.35f*shade);
                glBegin(GL_QUADS); glVertex2f(fx0+w*0.12f,fy0+h*0.12f); glVertex2f(fx1-w*0.12f,fy0+h*0.12f); glVertex2f(fx1-w*0.12f,fy1-h*0.12f); glVertex2f(fx0+w*0.12f,fy1-h*0.12f); glEnd();
                /* Cyan eye */
                glColor3f(0.20f*shade, 0.85f*shade, 0.90f*shade);
                glBegin(GL_QUADS); glVertex2f(midX-w*0.10f,(fy0+fy1)*0.5f-h*0.12f); glVertex2f(midX+w*0.10f,(fy0+fy1)*0.5f-h*0.12f); glVertex2f(midX+w*0.10f,(fy0+fy1)*0.5f+h*0.05f); glVertex2f(midX-w*0.10f,(fy0+fy1)*0.5f+h*0.05f); glEnd();
            }

            /* Health bar (2D, above sprite) */
            if (e->state != STATE_DYING && e->state != STATE_DEAD && tY < 21.0f) {
                float hr  = e->hp / e->maxHp;
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

static int enemyGetKillCount(void) { return gKillCount; }
static int enemyGetAliveCount(void) {
    int i, c = 0;
    for (i = 0; i < gNumEnemies; i++)
        if (gEnemies[i].active) c++;
    return c;
}

#endif /* ENEMY_H */
