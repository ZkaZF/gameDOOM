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
#define DETECT_RANGE   8.0f
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
            e->speed       = 0.016f;
            e->damage      = 10.0f;
            e->atkRange    = 0.80f;
            e->atkCooldown = 1.0f;
            break;
        case ENEMY_DEMON:
            e->hp          = 180.0f; e->maxHp = 180.0f;
            e->speed       = 0.009f;
            e->damage      = 35.0f;
            e->atkRange    = 0.90f;
            e->atkCooldown = 1.8f;
            break;
        case ENEMY_SPECTRE:
            e->hp          = 35.0f;  e->maxHp = 35.0f;
            e->speed       = 0.026f;
            e->damage      = 8.0f;
            e->atkRange    = 0.75f;
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

    /* 5 Imps — tersebar di berbagai ruangan (map 40x32) */
    enemySpawn(ENEMY_IMP,     4.5f, 15.5f);   /* left room   — tengah */
    enemySpawn(ENEMY_IMP,    15.5f,  4.5f);   /* top room    — tengah */
    enemySpawn(ENEMY_IMP,    31.0f, 11.0f);   /* right room  — pojok atas */
    enemySpawn(ENEMY_IMP,    31.0f, 20.0f);   /* right room  — pojok bawah */
    enemySpawn(ENEMY_IMP,    15.5f, 26.5f);   /* bottom room — tengah */

    /* 2 Demons — menjaga hub/center */
    enemySpawn(ENEMY_DEMON,  14.5f, 15.5f);   /* hub — sisi kiri */
    enemySpawn(ENEMY_DEMON,  17.5f, 15.5f);   /* hub — sisi kanan */

    /* 3 Spectres — di koridor + right room besar */
    enemySpawn(ENEMY_SPECTRE, 15.5f, 10.5f);  /* koridor atas */
    enemySpawn(ENEMY_SPECTRE, 15.5f, 21.5f);  /* koridor bawah */
    enemySpawn(ENEMY_SPECTRE, 10.5f, 15.5f);  /* koridor kiri */
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
                if (dist > DETECT_RANGE + 2.0f) {
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
                    if (e->type == ENEMY_SPECTRE && dist < 2.5f) {
                        nx = -(dx / dist) * spd * 0.6f;
                        ny = -(dy / dist) * spd * 0.6f;
                    } else {
                        nx = (dx / dist) * spd;
                        ny = (dy / dist) * spd;
                    }
                    /* Slide-style collision */
                    if (isWalkable(e->x + nx + (nx > 0 ? 0.3f : -0.3f), e->y))
                        e->x += nx;
                    if (isWalkable(e->x, e->y + ny + (ny > 0 ? 0.3f : -0.3f)))
                        e->y += ny;
                }
                break;

            case STATE_ATTACK:
                e->atkTimer += dt;
                if (e->atkTimer >= e->atkCooldown) {
                    e->atkTimer = 0.0f;
                    if (dist <= e->atkRange + 0.3f) {
                        /* Deal damage */
                        player->health -= (int)e->damage;
                        if (player->health < 0) player->health = 0;
                        gDamageFlash = 1.0f; /* red screen flash */
                    } else {
                        e->state = STATE_CHASE;
                    }
                }
                /* Chase again if target moves away */
                if (dist > e->atkRange + 0.6f)
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

    /* ── Single 3D perspective pass ── */
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    {
        GLfloat lPos[]  = { 0.0f, 1.0f, 0.0f, 0.0f };
        GLfloat lAmb[]  = { 0.38f, 0.33f, 0.28f, 1.0f };
        GLfloat lDiff[] = { 0.82f, 0.78f, 0.68f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, lPos);
        glLightfv(GL_LIGHT0, GL_AMBIENT,  lAmb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  lDiff);
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    /*
     * Match raycaster pitch using a frustum shift — NOT a camera tilt.
     * Tilting gluLookAt would rotate the world and make enemies float up/down.
     * Shifting the frustum vertically moves the "centre of the screen" exactly
     * the same way the raycaster's pitchOff pixel-shifts wall strips.
     *
     * tan(vFovHalf) = planeLen / aspect   (same derivation as gluPerspective match)
     * The half-height of the near plane  = near * tan(vFovHalf)
     * Pitch shift in near-plane units    = (pitch / halfScreenH) * halfNear
     *   (positive pitch => horizon down => frustum centre shifts down => enemies appear UP)
     */
    {
        double near_d       = 0.05;
        double far_d        = 30.0;
        double planeLen     = 0.66;
        double hFovTan      = planeLen;                      /* tan(hFovHalf) */
        double vFovTan      = planeLen / (double)aspect;     /* tan(vFovHalf) */
        double half_w       = near_d * hFovTan;
        double half_h       = near_d * vFovTan;
        double pitch_shift  = (player->pitch / (SCREEN_H * 0.5)) * half_h;

        glFrustum(-half_w, half_w,
                  -half_h + pitch_shift,
                   half_h + pitch_shift,
                   near_d, far_d);
    }

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    /* Camera looks straight ahead — NO pitch tilt here.
     * Vertical aim is handled entirely by the frustum shift above. */
    gluLookAt(
        (double)player->x, 0.5, (double)player->y,
        (double)(player->x + player->dirX), 0.5, (double)(player->y + player->dirY),
        0.0, 1.0, 0.0
    );

    /* Render sorted enemies */
    {
        int k;
        for (k = 0; k < n; k++) {
            Enemy* e = &gEnemies[sortIdx[k]];
            float  dx, dy, tX, tY;
            int    scrX;
            float  deathAnim, scale;

            dx = e->x - player->x;
            dy = e->y - player->y;

            /*
             * Correct sprite camera transform (Lode's formula):
             *   transformX = horizontal screen position  = (dirX*dy - dirY*dx) / det
             *   transformY = depth in front of camera    = (planeY*dx - planeX*dy) / det
             * Previous code had tX/tY swapped — causing enemies to appear at wrong
             * screen positions and pop through walls.
             */
            {
                float transformX = (player->dirX * dy - player->dirY * dx) / det;
                float transformY = (player->planeY * dx - player->planeX * dy) / det;

                if (transformY <= 0.1f) continue;  /* behind camera */

                scrX = (int)((float)(SCREEN_W / 2) * (1.0f + transformX / transformY));
                if (scrX < 1 || scrX >= SCREEN_W - 1) continue;

                /* Wall occlusion: sample columns across the enemy's projected width.
                 * halfW is proportional to projected screen size so large/close enemies
                 * don't incorrectly pop-in/out when their centre pixel is behind a wall. */
                {
                    int col, behindWall = 1;
                    int projH = abs((int)(SCREEN_H / transformY));
                    int halfW = projH / 3;     /* ~1/3 of projected height as half-width */
                    if (halfW < 4)  halfW = 4;  /* minimum 4-column scan */
                    if (halfW > 80) halfW = 80; /* cap to avoid huge loops */
                    {
                        int step = (halfW <= 8) ? 1 : halfW / 5;
                        for (col = -halfW; col <= halfW; col += step) {
                            int c = scrX + col;
                            if (c >= 0 && c < SCREEN_W && zBuffer[c] >= transformY * 0.90f) {
                                behindWall = 0;
                                break;
                            }
                        }
                    }
                    if (behindWall) continue;
                }

                /* Store corrected depth for health bar check below */
                tX = transformX;
                tY = transformY;
            }

            /* Death animation */
            deathAnim = 0.0f;
            if (e->state == STATE_DYING) {
                deathAnim = e->deathTimer / DEATH_DURATION;
                if (deathAnim > 1.0f) deathAnim = 1.0f;
            }

            scale = (e->type == ENEMY_DEMON)   ? 0.95f :
                    (e->type == ENEMY_SPECTRE)  ? 0.65f : 0.76f;

            glPushMatrix();
            glTranslatef(e->x, 0.0f, e->y);
            glScalef(scale, scale, scale);

            /* Rotate to face player (Y-axis billboard) */
            glRotatef(
                (float)(atan2f(dx, dy) * 180.0f / (float)M_PI),
                0.0f, 1.0f, 0.0f
            );

            /* Render model */
            switch (e->type) {
                case ENEMY_IMP:     renderImpModel(deathAnim);     break;
                case ENEMY_DEMON:   renderDemonModel(deathAnim);   break;
                case ENEMY_SPECTRE: renderSpectreModel(deathAnim); break;
            }

            /* Health bar (3D billboard quad — only if alive and close) */
            /* tY = corrected depth (set in transform block above) */
            if (e->state != STATE_DYING && e->state != STATE_DEAD && tY < 7.0f) {
                float hr = e->hp / e->maxHp;
                glDisable(GL_LIGHTING);

                /* Background */
                glColor3f(0.18f, 0.0f, 0.0f);
                glBegin(GL_QUADS);
                    glVertex3f(-0.36f, 1.05f, 0.0f);
                    glVertex3f( 0.36f, 1.05f, 0.0f);
                    glVertex3f( 0.36f, 1.14f, 0.0f);
                    glVertex3f(-0.36f, 1.14f, 0.0f);
                glEnd();

                /* Fill */
                if (hr > 0.5f)       glColor3f(0.12f, 0.82f, 0.22f);
                else if (hr > 0.25f) glColor3f(0.90f, 0.70f, 0.10f);
                else                 glColor3f(0.92f, 0.10f, 0.08f);
                glBegin(GL_QUADS);
                    glVertex3f(-0.36f, 1.05f, 0.0f);
                    glVertex3f(-0.36f + 0.72f * hr, 1.05f, 0.0f);
                    glVertex3f(-0.36f + 0.72f * hr, 1.14f, 0.0f);
                    glVertex3f(-0.36f, 1.14f, 0.0f);
                glEnd();
                glEnable(GL_LIGHTING);
            }

            glPopMatrix();
        }
    }

    /* Restore */
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();

    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_DEPTH_TEST);
}

static int enemyGetKillCount(void) { return gKillCount; }
static int enemyGetAliveCount(void) {
    int i, c = 0;
    for (i = 0; i < gNumEnemies; i++)
        if (gEnemies[i].active) c++;
    return c;
}

#endif /* ENEMY_H */
