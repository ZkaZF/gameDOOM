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
#define ITEM_RADIUS    1.65f   /* auto-pickup distance — 0.55f × MAP_SCALE(3) */
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
        /* Add some free health packs at start of new wave (positions × MAP_SCALE) */
        for (i = 0; i < 2; i++) {
            float px = 18.0f + (float)(rand() % 36);   /* 6+rand%12 × 3 */
            float py = 18.0f + (float)(rand() % 36);
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
    int i;
    float det = player->dirX * player->planeY - player->planeX * player->dirY;
    int pitchInt = (int)player->pitch;
    int horizY   = SCREEN_H / 2 + pitchInt;

    if (fabsf(det) < 0.00001f) det = 0.00001f;

    for (i = 0; i < gNumItems; i++) {
        Item* it = &gItems[i];
        float dx, dy, tX, tY;
        int   screenX, behindWall, col;
        float fullH, floorY, sScale, bob;
        int   spriteH, spriteW, sX0, sX1, sY0, sY1;
        float fx0, fx1, fy0, fy1, midX, w, h;
        float fadeAlpha = 1.0f;

        if (!it->active) continue;

        dx = it->x - player->x;
        dy = it->y - player->y;

        tX = (player->dirX * dy - player->dirY * dx) / det;
        tY = (player->planeY * dx - player->planeX * dy) / det;
        if (tY <= 0.1f) continue;

        screenX = (int)((float)(SCREEN_W / 2) * (1.0f + tX / tY));

        /* Fade out last 4 seconds of lifetime */
        if (it->lifetime > ITEM_LIFETIME - 4.0f) {
            fadeAlpha = (ITEM_LIFETIME - it->lifetime) / 4.0f;
            if (fadeAlpha < 0.0f) fadeAlpha = 0.0f;
        }

        /* Sprite dimensions — sits on floor */
        bob = sinf(it->bobPhase) * 0.05f * tY; /* Bobbing effect */
        fullH   = (float)SCREEN_H * WALL_HEIGHT_SCALE / tY;
        floorY  = (float)horizY + fullH * 0.5f;

        sScale = 0.40f; /* Base size for items */
        spriteH = (int)(fullH * sScale);
        if (spriteH < 2) spriteH = 2;
        spriteW = spriteH;

        sY1 = (int)floorY - (int)(fullH * bob);
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

        /* Clamp draw rect */
        fx0  = (float)(sX0 < 0 ? 0 : sX0);
        fx1  = (float)(sX1 >= SCREEN_W ? SCREEN_W-1 : sX1);
        fy0  = (float)(sY0 < 0 ? 0 : sY0);
        fy1  = (float)(sY1 >= SCREEN_H ? SCREEN_H-1 : sY1);
        midX = (fx0 + fx1) * 0.5f;
        w    = fx1 - fx0;
        h    = fy1 - fy0;
        if (w < 1 || h < 1) continue;

        if (fadeAlpha < 1.0f) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        if (it->type == ITEM_HEALTH) {
            /* Health Box: Red with white cross */
            glColor4f(0.85f, 0.10f, 0.10f, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(fx0,fy0); glVertex2f(fx1,fy0); glVertex2f(fx1,fy1); glVertex2f(fx0,fy1); glEnd();
            /* White cross */
            glColor4f(0.95f, 0.95f, 0.95f, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(midX-w*0.3f, fy0+h*0.4f); glVertex2f(midX+w*0.3f, fy0+h*0.4f); glVertex2f(midX+w*0.3f, fy0+h*0.6f); glVertex2f(midX-w*0.3f, fy0+h*0.6f); glEnd();
            glBegin(GL_QUADS); glVertex2f(midX-w*0.1f, fy0+h*0.2f); glVertex2f(midX+w*0.1f, fy0+h*0.2f); glVertex2f(midX+w*0.1f, fy0+h*0.8f); glVertex2f(midX-w*0.1f, fy0+h*0.8f); glEnd();
        } else if (it->type == ITEM_AMMO) {
            /* Ammo Box: Yellow */
            glColor4f(0.85f, 0.78f, 0.10f, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(fx0,fy0+h*0.2f); glVertex2f(fx1,fy0+h*0.2f); glVertex2f(fx1,fy1); glVertex2f(fx0,fy1); glEnd();
            /* Dark lid */
            glColor4f(0.72f, 0.64f, 0.08f, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(fx0,fy0); glVertex2f(fx1,fy0); glVertex2f(fx1,fy0+h*0.2f); glVertex2f(fx0,fy0+h*0.2f); glEnd();
        } else if (it->type == ITEM_ARMOR) {
            /* Armor: Teal shield */
            glColor4f(0.10f, 0.65f, 0.72f, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(midX-w*0.4f,fy0); glVertex2f(midX+w*0.4f,fy0); glVertex2f(midX+w*0.4f,fy1-h*0.2f); glVertex2f(midX-w*0.4f,fy1-h*0.2f); glEnd();
            glBegin(GL_TRIANGLES); glVertex2f(midX-w*0.4f,fy1-h*0.2f); glVertex2f(midX+w*0.4f,fy1-h*0.2f); glVertex2f(midX,fy1); glEnd();
            /* Emblem */
            glColor4f(0.90f, 0.90f, 0.25f, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(midX-w*0.15f,fy0+h*0.3f); glVertex2f(midX+w*0.15f,fy0+h*0.3f); glVertex2f(midX+w*0.15f,fy0+h*0.6f); glVertex2f(midX-w*0.15f,fy0+h*0.6f); glEnd();
        }

        if (fadeAlpha < 1.0f) glDisable(GL_BLEND);
    }
}

#endif /* ITEM_H */
