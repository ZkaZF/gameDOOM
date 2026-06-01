
#include "item.h"
#include "enemy.h"
#include "weapon.h"
#include "raycaster.h"
#include "map.h"
#include "player.h"
#include "texture.h"
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

Item  gItems[MAX_ITEMS];
int   gNumItems   = 0;
int   gPlayerScore = 0;
int   gWave        = 1;
float gWaveTimer   = 0.0f;

int itemGetScore(void) { return gPlayerScore; }
int itemGetWave(void)  { return gWave; }

void itemSpawn(int type, float x, float y) {
    int i;
    for (i = 0; i < MAX_ITEMS; i++) {
        if (!gItems[i].active) {
            gItems[i].type     = type;
            gItems[i].x        = x;
            gItems[i].y        = y;
            gItems[i].rotAngle = 0.0f;
            gItems[i].bobPhase = (float)(rand() % 628) / 100.0f;
            gItems[i].lifetime = 0.0f;
            gItems[i].active   = 1;
            if (i >= gNumItems) gNumItems = i + 1;
            return;
        }
    }
}

void itemTryDrop(float x, float y) {
    int roll = rand() % 100;
    if      (roll < 35) itemSpawn(ITEM_HEALTH, x, y);
    else if (roll < 85) itemSpawn(ITEM_AMMO,   x, y);
    else                itemSpawn(ITEM_ARMOR,   x, y);
}

void itemInit(void) {
    int i;
    memset(gItems, 0, sizeof(gItems));
    gNumItems    = 0;
    gPlayerScore = 0;
    gWave        = 1;
    gWaveTimer   = 0.0f;
    /* Spawn 3 weapon crates in hub room */
    /* Hub is near spawn (13.5, 70.5), place crates in a row */
    for (i = 0; i < 3; i++) {
        gItems[i].type       = ITEM_WEAPON_CRATE;
        gItems[i].weaponType = i;  /* 0=PISTOL, 1=SHOTGUN, 2=M416 */
        gItems[i].x          = 10.5f + (float)i * 3.0f;  /* 10.5, 13.5, 16.5 */
        gItems[i].y          = 67.5f;
        gItems[i].rotAngle   = 0.0f;
        gItems[i].bobPhase   = (float)i * 2.0f;
        gItems[i].lifetime   = 0.0f;
        gItems[i].active     = 1;
    }
    gNumItems = 3;
}

static void itemCheckWave(float dt) {
    /* Legacy wave system — DISABLED.
       Arena system (enemy.cpp) now handles all wave spawning.
       This used to call enemyInitLevel() which reset arena completed flags. */
    (void)dt;
}

static void itemApplyPickup(Item* it, Player* player) {
    switch (it->type) {
        case ITEM_HEALTH:
            player->health += 30;
            if (player->health > 100) player->health = 100;
            gPlayerScore += 10;
            break;
        case ITEM_AMMO: {
            /* Add to reserve ammo of current weapon */
            if (gCurrentWeapon != WEAPON_NONE) {
                Weapon* w = &gWeapons[gCurrentWeapon];
                w->reserveAmmo += w->maxAmmo;
                if (w->reserveAmmo > w->maxReserveAmmo)
                    w->reserveAmmo = w->maxReserveAmmo;
            }
            gPlayerScore += 5;
            break;
        }
        case ITEM_ARMOR:
            player->armor += 40;
            if (player->armor > 100) player->armor = 100;
            gPlayerScore += 15;
            break;
        case ITEM_WEAPON_CRATE:
            weaponUnlock(it->weaponType);
            printf("[Crate] Picked up weapon crate: %s\n",
                   gWeapons[it->weaponType].name);
            break;
    }
}

void itemUpdate(Player* player, float dt) {
    int i;
    itemCheckWave(dt);
    for (i = 0; i < gNumEnemies; i++) {
        Enemy* e = &gEnemies[i];
        if (e->state == STATE_DYING && e->deathTimer < dt * 1.5f && e->active) {
            itemTryDrop(e->x, e->y);
            gPlayerScore += 50 + (e->type == ENEMY_DEMON ? 100 : e->type == ENEMY_SPECTRE ? 30 : 0);
        }
    }
    for (i = 0; i < gNumItems; i++) {
        Item* it = &gItems[i];
        float dx, dy, dist;
        if (!it->active) continue;
        it->rotAngle += ITEM_ROT_SPEED * dt;
        if (it->rotAngle >= 360.0f) it->rotAngle -= 360.0f;
        it->bobPhase += ITEM_BOB_SPEED * dt;
        it->lifetime += dt;
        /* Weapon crates never expire */
        if (it->type != ITEM_WEAPON_CRATE && it->lifetime >= ITEM_LIFETIME) { it->active = 0; continue; }
        dx   = player->x - it->x;
        dy   = player->y - it->y;
        dist = sqrtf(dx*dx + dy*dy);
        if (dist <= ITEM_RADIUS) { itemApplyPickup(it, player); it->active = 0; }
    }
}

void renderItems(Player* player) {
    int i;
    float det    = player->dirX * player->planeY - player->planeX * player->dirY;
    int pitchInt = (int)(player->pitch + player->jumpZ * 120.0f);
    int horizY   = SCREEN_H / 2 + pitchInt;
    if (fabsf(det) < 0.00001f) det = 0.00001f;
    for (i = 0; i < gNumItems; i++) {
        Item* it = &gItems[i];
        float dx, dy, tX, tY, fullH, floorY, sScale, bob, fadeAlpha;
        int   screenX, behindWall, col, spriteH, spriteW, sX0, sX1, sY0, sY1;
        float fx0, fx1, fy0, fy1, midX, w, h;
        fadeAlpha = 1.0f;
        if (!it->active) continue;
        dx = it->x - player->x; dy = it->y - player->y;
        tX = (player->dirX * dy - player->dirY * dx) / det;
        tY = (player->planeY * dx - player->planeX * dy) / det;
        if (tY <= 0.1f) continue;
        screenX = (int)((float)(SCREEN_W / 2) * (1.0f + tX / tY));
        if (it->lifetime > ITEM_LIFETIME - 4.0f) {
            fadeAlpha = (ITEM_LIFETIME - it->lifetime) / 4.0f;
            if (fadeAlpha < 0.0f) fadeAlpha = 0.0f;
        }
        /* No bob for crates — always static */
        bob     = (it->type == ITEM_WEAPON_CRATE) ? 0.0f : sinf(it->bobPhase) * 0.05f * tY;
        fullH   = (float)SCREEN_H * WALL_HEIGHT_SCALE / tY;
        floorY  = (float)horizY + fullH * 0.5f;
        sScale  = 0.40f;
        spriteH = (int)(fullH * sScale);
        if (spriteH < 2) spriteH = 2;
        spriteW = spriteH;
        sY1 = (int)floorY - (int)(fullH * bob);
        sY0 = sY1 - spriteH;
        sX0 = screenX - spriteW/2; sX1 = screenX + spriteW/2;
        if (sX1 < 0 || sX0 >= SCREEN_W || sY1 < 0 || sY0 >= SCREEN_H) continue;
        behindWall = 1;
        { int step = (spriteW > 16) ? spriteW/6 : 1;
          for (col = sX0; col <= sX1; col += step)
              if (col >= 0 && col < SCREEN_W && zBuffer[col] >= tY * 0.90f) { behindWall = 0; break; } }
        if (behindWall) continue;
        fx0 = (float)(sX0 < 0 ? 0 : sX0);
        fx1 = (float)(sX1 >= SCREEN_W ? SCREEN_W-1 : sX1);
        fy0 = (float)(sY0 < 0 ? 0 : sY0);
        fy1 = (float)(sY1 >= SCREEN_H ? SCREEN_H-1 : sY1);
        midX = (fx0 + fx1) * 0.5f;
        w = fx1 - fx0; h = fy1 - fy0;
        if (w < 1 || h < 1) continue;
        if (fadeAlpha < 1.0f) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); }
        if (it->type == ITEM_HEALTH) {
            glColor4f(0.85f, 0.10f, 0.10f, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(fx0,fy0); glVertex2f(fx1,fy0); glVertex2f(fx1,fy1); glVertex2f(fx0,fy1); glEnd();
            glColor4f(0.95f, 0.95f, 0.95f, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(midX-w*0.3f,fy0+h*0.4f); glVertex2f(midX+w*0.3f,fy0+h*0.4f); glVertex2f(midX+w*0.3f,fy0+h*0.6f); glVertex2f(midX-w*0.3f,fy0+h*0.6f); glEnd();
            glBegin(GL_QUADS); glVertex2f(midX-w*0.1f,fy0+h*0.2f); glVertex2f(midX+w*0.1f,fy0+h*0.2f); glVertex2f(midX+w*0.1f,fy0+h*0.8f); glVertex2f(midX-w*0.1f,fy0+h*0.8f); glEnd();
        } else if (it->type == ITEM_AMMO) {
            glColor4f(0.85f, 0.78f, 0.10f, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(fx0,fy0+h*0.2f); glVertex2f(fx1,fy0+h*0.2f); glVertex2f(fx1,fy1); glVertex2f(fx0,fy1); glEnd();
            glColor4f(0.72f, 0.64f, 0.08f, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(fx0,fy0); glVertex2f(fx1,fy0); glVertex2f(fx1,fy0+h*0.2f); glVertex2f(fx0,fy0+h*0.2f); glEnd();
        } else if (it->type == ITEM_ARMOR) {
            glColor4f(0.10f, 0.65f, 0.72f, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(midX-w*0.4f,fy0); glVertex2f(midX+w*0.4f,fy0); glVertex2f(midX+w*0.4f,fy1-h*0.2f); glVertex2f(midX-w*0.4f,fy1-h*0.2f); glEnd();
            glBegin(GL_TRIANGLES); glVertex2f(midX-w*0.4f,fy1-h*0.2f); glVertex2f(midX+w*0.4f,fy1-h*0.2f); glVertex2f(midX,fy1); glEnd();
            glColor4f(0.90f, 0.90f, 0.25f, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(midX-w*0.15f,fy0+h*0.3f); glVertex2f(midX+w*0.15f,fy0+h*0.3f); glVertex2f(midX+w*0.15f,fy0+h*0.6f); glVertex2f(midX-w*0.15f,fy0+h*0.6f); glEnd();
        } else if (it->type == ITEM_WEAPON_CRATE) {
            /* 3D weapon lying on floor — static, no bob */
            float wScale = fullH * 0.35f;  /* size relative to wall */
            float cy_bot = (float)horizY + fullH * 0.5f;
            float cy_mid = cy_bot - wScale * 0.25f;  /* weapon sits near floor */
            float cy_top = cy_bot - wScale * 0.50f;
            float cx_cen = (float)screenX;
            /* Glow outline color per weapon */
            float gr, gg, gb;
            if (it->weaponType == WEAPON_PISTOL)       { gr=0.90f; gg=0.90f; gb=0.90f; }
            else if (it->weaponType == WEAPON_SHOTGUN)  { gr=1.00f; gg=0.60f; gb=0.10f; }
            else                                        { gr=0.10f; gg=0.85f; gb=0.30f; }
            /* Pulsing glow */
            {
                double t = (double)glutGet(GLUT_ELAPSED_TIME) / 1000.0;
                float pulse = 0.5f + 0.5f * (float)sin(t * 3.0);
                float glowA = 0.25f + pulse * 0.20f;
                float glowR = wScale * 0.55f;
                /* Glow circle behind weapon */
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glColor4f(gr, gg, gb, glowA);
                glBegin(GL_TRIANGLE_FAN);
                    glVertex2f(cx_cen, (cy_top+cy_bot)*0.5f);
                    { int seg; for (seg = 0; seg <= 20; seg++) {
                        float a = (float)seg / 20.0f * 6.2832f;
                        glVertex2f(cx_cen + cosf(a)*glowR*1.3f,
                                   (cy_top+cy_bot)*0.5f + sinf(a)*glowR*0.5f);
                    }}
                glEnd();
            }
            if (it->weaponType == WEAPON_PISTOL) {
                /* Pistol: small compact gun */
                float pw = wScale * 0.40f, ph = wScale * 0.12f;
                /* Barrel */
                glColor3f(0.25f, 0.25f, 0.28f);
                glBegin(GL_QUADS);
                    glVertex2f(cx_cen - pw*0.1f, cy_mid - ph);
                    glVertex2f(cx_cen + pw,      cy_mid - ph);
                    glVertex2f(cx_cen + pw,      cy_mid + ph*0.5f);
                    glVertex2f(cx_cen - pw*0.1f, cy_mid + ph*0.5f);
                glEnd();
                /* Grip */
                glColor3f(0.35f, 0.22f, 0.10f);
                glBegin(GL_QUADS);
                    glVertex2f(cx_cen - pw*0.15f, cy_mid + ph*0.5f);
                    glVertex2f(cx_cen + pw*0.15f, cy_mid + ph*0.5f);
                    glVertex2f(cx_cen + pw*0.05f, cy_mid + ph*2.5f);
                    glVertex2f(cx_cen - pw*0.25f, cy_mid + ph*2.5f);
                glEnd();
                /* Outline */
                glColor3f(gr*0.8f, gg*0.8f, gb*0.8f);
                glLineWidth(1.5f);
                glBegin(GL_LINE_LOOP);
                    glVertex2f(cx_cen - pw*0.1f, cy_mid - ph);
                    glVertex2f(cx_cen + pw,      cy_mid - ph);
                    glVertex2f(cx_cen + pw,      cy_mid + ph*0.5f);
                    glVertex2f(cx_cen - pw*0.1f, cy_mid + ph*0.5f);
                glEnd();
                glLineWidth(1.0f);
            } else if (it->weaponType == WEAPON_SHOTGUN) {
                /* Shotgun: long barrel + wooden stock */
                float sw = wScale * 0.65f, sh = wScale * 0.10f;
                /* Barrel */
                glColor3f(0.30f, 0.30f, 0.33f);
                glBegin(GL_QUADS);
                    glVertex2f(cx_cen - sw*0.2f, cy_mid - sh);
                    glVertex2f(cx_cen + sw,      cy_mid - sh);
                    glVertex2f(cx_cen + sw,      cy_mid + sh*0.6f);
                    glVertex2f(cx_cen - sw*0.2f, cy_mid + sh*0.6f);
                glEnd();
                /* Pump/forend */
                glColor3f(0.20f, 0.20f, 0.22f);
                glBegin(GL_QUADS);
                    glVertex2f(cx_cen + sw*0.3f, cy_mid - sh*1.3f);
                    glVertex2f(cx_cen + sw*0.6f, cy_mid - sh*1.3f);
                    glVertex2f(cx_cen + sw*0.6f, cy_mid + sh*0.9f);
                    glVertex2f(cx_cen + sw*0.3f, cy_mid + sh*0.9f);
                glEnd();
                /* Stock */
                glColor3f(0.45f, 0.28f, 0.12f);
                glBegin(GL_QUADS);
                    glVertex2f(cx_cen - sw*0.7f, cy_mid - sh*0.6f);
                    glVertex2f(cx_cen - sw*0.2f, cy_mid - sh*0.6f);
                    glVertex2f(cx_cen - sw*0.2f, cy_mid + sh*1.2f);
                    glVertex2f(cx_cen - sw*0.7f, cy_mid + sh*1.8f);
                glEnd();
                /* Outline */
                glColor3f(gr*0.8f, gg*0.8f, gb*0.8f);
                glLineWidth(1.5f);
                glBegin(GL_LINE_LOOP);
                    glVertex2f(cx_cen - sw*0.7f, cy_mid - sh*0.6f);
                    glVertex2f(cx_cen + sw,      cy_mid - sh);
                    glVertex2f(cx_cen + sw,      cy_mid + sh*0.6f);
                    glVertex2f(cx_cen - sw*0.7f, cy_mid + sh*1.8f);
                glEnd();
                glLineWidth(1.0f);
            } else {
                /* M416: rifle with magazine */
                float rw = wScale * 0.60f, rh = wScale * 0.09f;
                /* Barrel + receiver */
                glColor3f(0.18f, 0.18f, 0.20f);
                glBegin(GL_QUADS);
                    glVertex2f(cx_cen - rw*0.15f, cy_mid - rh);
                    glVertex2f(cx_cen + rw,       cy_mid - rh);
                    glVertex2f(cx_cen + rw,       cy_mid + rh*0.8f);
                    glVertex2f(cx_cen - rw*0.15f, cy_mid + rh*0.8f);
                glEnd();
                /* Magazine */
                glColor3f(0.12f, 0.12f, 0.14f);
                glBegin(GL_QUADS);
                    glVertex2f(cx_cen + rw*0.25f, cy_mid + rh*0.8f);
                    glVertex2f(cx_cen + rw*0.40f, cy_mid + rh*0.8f);
                    glVertex2f(cx_cen + rw*0.38f, cy_mid + rh*3.0f);
                    glVertex2f(cx_cen + rw*0.23f, cy_mid + rh*3.0f);
                glEnd();
                /* Stock */
                glColor3f(0.22f, 0.22f, 0.24f);
                glBegin(GL_QUADS);
                    glVertex2f(cx_cen - rw*0.55f, cy_mid - rh*0.5f);
                    glVertex2f(cx_cen - rw*0.15f, cy_mid - rh*0.5f);
                    glVertex2f(cx_cen - rw*0.15f, cy_mid + rh*1.2f);
                    glVertex2f(cx_cen - rw*0.55f, cy_mid + rh*1.5f);
                glEnd();
                /* Grip */
                glColor3f(0.15f, 0.15f, 0.17f);
                glBegin(GL_QUADS);
                    glVertex2f(cx_cen + rw*0.05f, cy_mid + rh*0.8f);
                    glVertex2f(cx_cen + rw*0.18f, cy_mid + rh*0.8f);
                    glVertex2f(cx_cen + rw*0.15f, cy_mid + rh*2.5f);
                    glVertex2f(cx_cen + rw*0.02f, cy_mid + rh*2.5f);
                glEnd();
                /* Outline */
                glColor3f(gr*0.8f, gg*0.8f, gb*0.8f);
                glLineWidth(1.5f);
                glBegin(GL_LINE_LOOP);
                    glVertex2f(cx_cen - rw*0.55f, cy_mid - rh*0.5f);
                    glVertex2f(cx_cen + rw,       cy_mid - rh);
                    glVertex2f(cx_cen + rw,       cy_mid + rh*0.8f);
                    glVertex2f(cx_cen - rw*0.55f, cy_mid + rh*1.5f);
                glEnd();
                glLineWidth(1.0f);
            }
            glDisable(GL_BLEND);
        }
        if (fadeAlpha < 1.0f) glDisable(GL_BLEND);
    }
}
