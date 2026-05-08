#ifndef ITEM_H
#define ITEM_H

/*
 * item.h — Item & Pickup System (V4)
 *
 * Item Types:
 *   ITEM_HEALTH  — +30 HP   (red cross box)
 *   ITEM_AMMO    — +ammo    (yellow crate)
 *   ITEM_ARMOR   — +40 armor (teal shield torus)
 *
 * Features:
 *   - Random drop on enemy death (probabilities per type)
 *   - Floating + rotating 3D animation
 *   - Auto-pickup when player walks within radius
 *   - Score system (points per kill + per item)
 *   - Wave system: new wave spawns when all enemies dead
 *
 * Depends (included before this in main.cpp):
 *   map.h, player.h, raycaster.h, weapon.h, enemy.h
 */

#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ───────────────────── Constants ───────────────────── */
#define ITEM_HEALTH  0
#define ITEM_AMMO    1
#define ITEM_ARMOR   2
#define NUM_ITEM_TYPES 3

#define MAX_ITEMS      32
#define ITEM_RADIUS    0.55f   /* auto-pickup distance */
#define ITEM_BOB_SPEED 2.4f    /* bobbing speed (rad/s) */
#define ITEM_ROT_SPEED 90.0f   /* rotation speed (deg/s) */
#define ITEM_LIFETIME  25.0f   /* seconds before item despawns */

/* ───────────────────── Struct ───────────────────── */
typedef struct {
    int   type;
    float x, y;
    float rotAngle;    /* current Y-rotation (degrees) */
    float bobPhase;    /* current bobbing phase (radians) */
    float lifetime;    /* seconds alive */
    int   active;
} Item;

/* ───────────────────── Globals ───────────────────── */
static Item  gItems[MAX_ITEMS];
static int   gNumItems  = 0;
static int   gPlayerScore = 0;
static int   gWave        = 1;
static float gWaveTimer   = 0.0f;   /* countdown after all enemies dead */

/* ───────────────────── Score & Wave Getters ───────────────────── */
static int   itemGetScore(void)    { return gPlayerScore; }
static int   itemGetWave(void)     { return gWave; }

/* ───────────────────── Item Spawn ───────────────────── */
static void itemSpawn(int type, float x, float y) {
    int i;
    for (i = 0; i < MAX_ITEMS; i++) {
        if (!gItems[i].active) {
            gItems[i].type      = type;
            gItems[i].x         = x;
            gItems[i].y         = y;
            gItems[i].rotAngle  = 0.0f;
            gItems[i].bobPhase  = (float)(rand() % 628) / 100.0f; /* random start phase */
            gItems[i].lifetime  = 0.0f;
            gItems[i].active    = 1;
            if (i >= gNumItems) gNumItems = i + 1;
            return;
        }
    }
}

/* ───────────────────── Drop on Enemy Death ───────────────────── */
/*
 * Call this when an enemy state becomes STATE_DYING.
 * Drop chances: Health 35%, Ammo 50%, Armor 15%
 */
static void itemTryDrop(float x, float y) {
    int roll = rand() % 100;
    /* Always drop something for non-empty cells */
    if      (roll < 35) itemSpawn(ITEM_HEALTH, x, y);
    else if (roll < 85) itemSpawn(ITEM_AMMO,   x, y);
    else                itemSpawn(ITEM_ARMOR,   x, y);
}

/* ───────────────────── Init ───────────────────── */
static void itemInit(void) {
    memset(gItems, 0, sizeof(gItems));
    gNumItems    = 0;
    gPlayerScore = 0;
    gWave        = 1;
    gWaveTimer   = 0.0f;
}

/* ───────────────────── Wave System ───────────────────── */
static void itemCheckWave(float dt) {
    int alive = enemyGetAliveCount();
    if (alive > 0) {
        gWaveTimer = 0.0f;
        return;
    }
    /* All enemies dead — count down then spawn next wave */
    gWaveTimer += dt;
    if (gWaveTimer >= 5.0f) {
        int i;
        gWave++;
        gWaveTimer = 0.0f;
        /* Scale difficulty: more enemies each wave */
        enemyInitLevel();
        /* Bonus score for completing wave */
        gPlayerScore += gWave * 100;
        /* Add some free health packs at start of new wave */
        for (i = 0; i < 2; i++) {
            float px = 6.0f + (float)(rand() % 12);
            float py = 6.0f + (float)(rand() % 12);
            if (isWalkable((int)px, (int)py))
                itemSpawn(ITEM_HEALTH, px, py);
        }
    }
}

/* ───────────────────── Pickup Effect ───────────────────── */
static void itemApplyPickup(Item* it, Player* player) {
    switch (it->type) {
        case ITEM_HEALTH:
            player->health += 30;
            if (player->health > 100) player->health = 100;
            gPlayerScore += 10;
            break;
        case ITEM_AMMO: {
            /* Add ammo to current weapon */
            Weapon* w = &gWeapons[gCurrentWeapon];
            w->ammo += w->maxAmmo / 2;
            if (w->ammo > w->maxAmmo * 2) w->ammo = w->maxAmmo * 2; /* allow over-capacity */
            gPlayerScore += 5;
            break;
        }
        case ITEM_ARMOR:
            player->armor += 40;
            if (player->armor > 100) player->armor = 100;
            gPlayerScore += 15;
            break;
    }
}

/* ───────────────────── Update ───────────────────── */
static void itemUpdate(Player* player, float dt) {
    int i;

    /* Check wave progression */
    itemCheckWave(dt);

    /* Check for new enemy deaths and drop items */
    for (i = 0; i < gNumEnemies; i++) {
        Enemy* e = &gEnemies[i];
        /* Drop on first frame of DYING state */
        if (e->state == STATE_DYING && e->deathTimer < dt * 1.5f && e->active) {
            itemTryDrop(e->x, e->y);
            gPlayerScore += 50 + (e->type == ENEMY_DEMON ? 100 : e->type == ENEMY_SPECTRE ? 30 : 0);
        }
    }

    /* Animate and check pickup for each item */
    for (i = 0; i < gNumItems; i++) {
        Item* it = &gItems[i];
        float dx, dy, dist;
        if (!it->active) continue;

        it->rotAngle  += ITEM_ROT_SPEED * dt;
        if (it->rotAngle >= 360.0f) it->rotAngle -= 360.0f;
        it->bobPhase  += ITEM_BOB_SPEED * dt;
        it->lifetime  += dt;

        /* Despawn after lifetime */
        if (it->lifetime >= ITEM_LIFETIME) {
            it->active = 0;
            continue;
        }

        /* Proximity pickup */
        dx   = player->x - it->x;
        dy   = player->y - it->y;
        dist = sqrtf(dx * dx + dy * dy);
        if (dist <= ITEM_RADIUS) {
            itemApplyPickup(it, player);
            it->active = 0;
        }
    }
}

/* ═══════════════════ 3D ITEM MODELS ═══════════════════ */

/* ── Health Pack (red box with white cross) ── */
static void renderHealthModel(float bob) {
    glPushMatrix();
    glTranslatef(0.0f, 0.18f + bob, 0.0f);

    /* Main red box */
    glColor3f(0.85f, 0.10f, 0.10f);
    glPushMatrix(); glScalef(0.30f, 0.30f, 0.30f); glutSolidCube(1.0f); glPopMatrix();

    /* White cross — horizontal bar */
    glColor3f(0.95f, 0.95f, 0.95f);
    glPushMatrix(); glScalef(0.26f, 0.08f, 0.07f); glutSolidCube(1.0f); glPopMatrix();
    /* Vertical bar */
    glPushMatrix(); glScalef(0.08f, 0.26f, 0.07f); glutSolidCube(1.0f); glPopMatrix();

    /* Slight glow edge */
    glColor3f(1.0f, 0.45f, 0.45f);
    glPushMatrix(); glScalef(0.32f, 0.32f, 0.32f); glutWireCube(1.0f); glPopMatrix();

    glPopMatrix();
}

/* ── Ammo Box (yellow crate) ── */
static void renderAmmoModel(float bob) {
    glPushMatrix();
    glTranslatef(0.0f, 0.16f + bob, 0.0f);

    /* Main yellow box */
    glColor3f(0.85f, 0.78f, 0.10f);
    glPushMatrix(); glScalef(0.34f, 0.22f, 0.26f); glutSolidCube(1.0f); glPopMatrix();

    /* Darker lid */
    glColor3f(0.72f, 0.64f, 0.08f);
    glPushMatrix(); glTranslatef(0.0f, 0.12f, 0.0f); glScalef(0.36f, 0.06f, 0.28f); glutSolidCube(1.0f); glPopMatrix();

    /* Bullet symbol (small dark rectangle on front) */
    glColor3f(0.22f, 0.18f, 0.04f);
    glPushMatrix(); glTranslatef(0.0f, 0.0f, 0.14f); glScalef(0.10f, 0.16f, 0.02f); glutSolidCube(1.0f); glPopMatrix();

    glPopMatrix();
}

/* ── Armor (teal rotating torus-like shape) ── */
static void renderArmorModel(float bob) {
    glPushMatrix();
    glTranslatef(0.0f, 0.24f + bob, 0.0f);

    /* Shield base */
    glColor3f(0.10f, 0.65f, 0.72f);
    glPushMatrix(); glScalef(0.28f, 0.36f, 0.14f); glutSolidCube(1.0f); glPopMatrix();

    /* Top arc */
    glColor3f(0.15f, 0.80f, 0.88f);
    if (gQuad) {
        glPushMatrix();
            glTranslatef(0.0f, 0.20f, 0.0f);
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(gQuad, 0.14f, 0.10f, 0.10f, 10, 1);
        glPopMatrix();
    }

    /* Star/emblem (smaller cube) */
    glColor3f(0.90f, 0.90f, 0.25f);
    glPushMatrix(); glScalef(0.09f, 0.12f, 0.08f); glutSolidCube(1.0f); glPopMatrix();

    /* Glow outline */
    glColor3f(0.35f, 0.95f, 1.0f);
    glPushMatrix(); glScalef(0.30f, 0.38f, 0.16f); glutWireCube(1.0f); glPopMatrix();

    glPopMatrix();
}

/* ═══════════════════ RENDER PASS ═══════════════════ */
static void renderItems(Player* player) {
    int  i;
    float det    = player->dirX * player->planeY - player->planeX * player->dirY;
    float aspect = (float)SCREEN_W / (float)SCREEN_H;

    if (fabsf(det) < 0.00001f) det = 0.00001f;

    /* ── 3D perspective pass matching raycaster + pitch ── */
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    {
        GLfloat lPos[]  = { 0.0f, 1.5f, 0.0f, 0.0f };
        GLfloat lAmb[]  = { 0.45f, 0.42f, 0.36f, 1.0f };
        GLfloat lDiff[] = { 0.88f, 0.82f, 0.70f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, lPos);
        glLightfv(GL_LIGHT0, GL_AMBIENT,  lAmb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  lDiff);
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    /* Frustum shift to match raycaster pitch */
    {
        double near_d   = 0.05;
        double far_d    = 30.0;
        double planeLen = 0.66;
        double half_w   = near_d * planeLen;
        double half_h   = near_d * (planeLen / (double)aspect);
        double ps       = (player->pitch / (SCREEN_H * 0.5)) * half_h;
        glFrustum(-half_w, half_w, -half_h + ps, half_h + ps, near_d, far_d);
    }

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    gluLookAt(
        (double)player->x, 0.5, (double)player->y,
        (double)(player->x + player->dirX), 0.5, (double)(player->y + player->dirY),
        0.0, 1.0, 0.0
    );

    for (i = 0; i < gNumItems; i++) {
        Item*  it = &gItems[i];
        float  dx, dy;
        float  transformX, transformY;
        int    scrX;
        float  bob;
        float  fadeAlpha;

        if (!it->active) continue;

        dx = it->x - player->x;
        dy = it->y - player->y;

        /* Camera-space transform for z-buffer occlusion check */
        transformX = (player->dirX * dy - player->dirY * dx) / det;
        transformY = (player->planeY * dx - player->planeX * dy) / det;

        if (transformY <= 0.1f) continue; /* behind camera */

        scrX = (int)((float)(SCREEN_W / 2) * (1.0f + transformX / transformY));
        if (scrX < 1 || scrX >= SCREEN_W - 1) continue;

        /* Wall occlusion: sample columns proportional to projected size */
        {
            int col, behindWall = 1;
            int projH = abs((int)(SCREEN_H / transformY));
            int halfW = projH / 3;
            if (halfW < 4)  halfW = 4;
            if (halfW > 60) halfW = 60;
            {
                int step = (halfW <= 8) ? 1 : halfW / 5;
                for (col = -halfW; col <= halfW; col += step) {
                    int c = scrX + col;
                    if (c >= 0 && c < SCREEN_W && zBuffer[c] >= transformY * 0.90f) {
                        behindWall = 0; break;
                    }
                }
            }
            if (behindWall) continue;
        }

        /* Bobbing height */
        bob = sinf(it->bobPhase) * 0.07f;

        /* Fade out last 4 seconds of lifetime */
        fadeAlpha = 1.0f;
        if (it->lifetime > ITEM_LIFETIME - 4.0f) {
            fadeAlpha = (ITEM_LIFETIME - it->lifetime) / 4.0f;
        }

        glPushMatrix();
        glTranslatef(it->x, 0.0f, it->y);
        glRotatef(it->rotAngle, 0.0f, 1.0f, 0.0f);

        if (fadeAlpha < 1.0f) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        switch (it->type) {
            case ITEM_HEALTH: renderHealthModel(bob); break;
            case ITEM_AMMO:   renderAmmoModel(bob);   break;
            case ITEM_ARMOR:  renderArmorModel(bob);  break;
        }

        if (fadeAlpha < 1.0f) glDisable(GL_BLEND);

        glPopMatrix();
    }

    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();

    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_DEPTH_TEST);
}

#endif /* ITEM_H */
