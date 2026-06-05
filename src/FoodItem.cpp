
#include "FoodItem.h"
#include "player.h"
#include "raycaster.h"
#include "weapon.h"
#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

FoodItem gFoodItems[MAX_FOOD_ITEMS];
int      gNumFoodItems    = 0;
int      gFoodCollected[FOOD_TYPE_COUNT] = {0, 0, 0, 0};
int      gOmprengCollected = 0;

/* Menginisialisasi semua data item makanan dan reset status yang sudah dikumpulkan */
void foodInit(void) {
    memset(gFoodItems, 0, sizeof(gFoodItems));
    gNumFoodItems = 0;
    memset(gFoodCollected, 0, sizeof(gFoodCollected));
    gOmprengCollected = 0;
}

/* Memunculkan item makanan dengan jenis tertentu di koordinat (x,y) */
void foodSpawn(int type, float x, float y) {
    int i;
    for (i = 0; i < MAX_FOOD_ITEMS; i++) {
        if (!gFoodItems[i].active) {
            gFoodItems[i].x        = x;
            gFoodItems[i].y        = y;
            gFoodItems[i].type     = type;
            gFoodItems[i].bobPhase = (float)(rand() % 628) / 100.0f;
            gFoodItems[i].lifetime = 0.0f;
            gFoodItems[i].active   = 1;
            if (i >= gNumFoodItems) gNumFoodItems = i + 1;
            return;
        }
    }
}

/* Menambahkan health/armor pemain sesuai dengan tipe makanan yang diambil */
static void foodApplyPickup(int type, Player *player) {
    gFoodCollected[type] = 1;
    switch (type) {
        case FOOD_SUSU_KOTAK:
            player->health += 30;
            if (player->health > 100) player->health = 100;
            break;
        case FOOD_NASI_PUTIH:
            player->health += 20;
            if (player->health > 100) player->health = 100;
            break;
        case FOOD_AYAM_GORENG:
            player->health += 40;
            if (player->health > 100) player->health = 100;
            player->armor  += 10;
            if (player->armor > 100) player->armor = 100;
            break;
        case FOOD_TELUR_CEPLOK:
            player->health += 25;
            if (player->health > 100) player->health = 100;
            break;
    }
}

/* Mengupdate efek animasi bobbing, mengecek kadaluarsa (lifetime), dan deteksi jika player menyentuh makanan */
void foodUpdate(Player *player, float dt) {
    int i;
    for (i = 0; i < gNumFoodItems; i++) {
        FoodItem *fi = &gFoodItems[i];
        float dx, dy, dist;
        if (!fi->active) continue;
        fi->bobPhase += FOOD_BOB_SPEED * dt;
        fi->lifetime += dt;
        if (fi->lifetime >= FOOD_LIFETIME) { fi->active = 0; continue; }
        dx   = player->x - fi->x;
        dy   = player->y - fi->y;
        dist = sqrtf(dx * dx + dy * dy);
        if (dist <= FOOD_PICKUP_R) {
            foodApplyPickup(fi->type, player);
            fi->active = 0;
            printf("[FoodItem] Picked up type %d\n", fi->type);
        }
    }
}

/* Mengecek apakah semua variasi makanan dan ompreng telah dikumpulkan (Syarat Menang) */
int foodAllCollected(void) {
    int i;
    for (i = 0; i < FOOD_TYPE_COUNT; i++) {
        if (!gFoodCollected[i]) return 0;
    }
    if (!gOmprengCollected) return 0;
    return 1;
}

/* ============================================================
   2D billboard rendering for each food type
   ============================================================ */

static void renderSusuKotak(float fx0, float fy0, float w, float h, float alpha) {
    float midX = fx0 + w * 0.5f;
    float L = fx0 + w*0.18f, R = fx0 + w*0.82f;
    float T = fy0 + h*0.18f, B = fy0 + h*0.95f;
    /* Kotak body — putih susu */
    glColor4f(0.96f, 0.96f, 0.99f, alpha);
    glBegin(GL_QUADS);
    glVertex2f(L, T); glVertex2f(R, T);
    glVertex2f(R, B); glVertex2f(L, B);
    glEnd();
    /* Atap lipatan segitiga */
    glColor4f(0.88f, 0.88f, 0.92f, alpha);
    glBegin(GL_TRIANGLES);
    glVertex2f(L, T); glVertex2f(R, T);
    glVertex2f(midX, fy0 + h*0.06f);
    glEnd();
    /* Strip biru branding area */
    glColor4f(0.15f, 0.45f, 0.92f, alpha);
    glBegin(GL_QUADS);
    glVertex2f(L, fy0+h*0.38f); glVertex2f(R, fy0+h*0.38f);
    glVertex2f(R, fy0+h*0.62f); glVertex2f(L, fy0+h*0.62f);
    glEnd();
    /* Tulisan "SUSU" placeholder — garis putih */
    glColor4f(0.95f, 0.95f, 1.0f, alpha * 0.9f);
    glBegin(GL_QUADS);
    glVertex2f(midX-w*0.18f, fy0+h*0.46f); glVertex2f(midX+w*0.18f, fy0+h*0.46f);
    glVertex2f(midX+w*0.18f, fy0+h*0.50f); glVertex2f(midX-w*0.18f, fy0+h*0.50f);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(midX-w*0.12f, fy0+h*0.52f); glVertex2f(midX+w*0.12f, fy0+h*0.52f);
    glVertex2f(midX+w*0.12f, fy0+h*0.55f); glVertex2f(midX-w*0.12f, fy0+h*0.55f);
    glEnd();
    /* Outline kotak */
    glColor4f(0.60f, 0.60f, 0.65f, alpha);
    glBegin(GL_LINE_LOOP);
    glVertex2f(L, T); glVertex2f(R, T);
    glVertex2f(R, B); glVertex2f(L, B);
    glEnd();
    /* Sedotan bengkok — merah */
    glColor4f(0.92f, 0.18f, 0.18f, alpha);
    glBegin(GL_QUADS);
    /* Bagian atas sedotan (vertikal) */
    glVertex2f(midX+w*0.05f, fy0); glVertex2f(midX+w*0.10f, fy0);
    glVertex2f(midX+w*0.10f, fy0+h*0.10f); glVertex2f(midX+w*0.05f, fy0+h*0.10f);
    glEnd();
    glBegin(GL_QUADS);
    /* Bagian bawah sedotan (miring masuk kotak) */
    glVertex2f(midX+w*0.02f, fy0+h*0.10f); glVertex2f(midX+w*0.10f, fy0+h*0.10f);
    glVertex2f(midX+w*0.06f, fy0+h*0.22f); glVertex2f(midX-w*0.02f, fy0+h*0.22f);
    glEnd();
}

static void renderNasiPutih(float fx0, float fy0, float w, float h, float alpha) {
    float midX = fx0 + w * 0.5f;
    /* Onigiri — clean rounded triangle using GL_POLYGON (no bounding-box fill) */
    /* White nasi body */
    glColor4f(0.98f, 0.98f, 0.94f, alpha);
    glBegin(GL_POLYGON);
    /* Bottom edge — flat */
    glVertex2f(fx0 + w*0.08f, fy0 + h*0.90f);
    glVertex2f(fx0 + w*0.92f, fy0 + h*0.90f);
    /* Right side — curve up */
    glVertex2f(fx0 + w*0.90f, fy0 + h*0.76f);
    glVertex2f(fx0 + w*0.84f, fy0 + h*0.60f);
    glVertex2f(fx0 + w*0.76f, fy0 + h*0.45f);
    glVertex2f(fx0 + w*0.66f, fy0 + h*0.30f);
    /* Rounded top */
    glVertex2f(fx0 + w*0.58f, fy0 + h*0.18f);
    glVertex2f(fx0 + w*0.52f, fy0 + h*0.13f);
    glVertex2f(midX,          fy0 + h*0.11f);
    glVertex2f(fx0 + w*0.48f, fy0 + h*0.13f);
    glVertex2f(fx0 + w*0.42f, fy0 + h*0.18f);
    /* Left side — curve down */
    glVertex2f(fx0 + w*0.34f, fy0 + h*0.30f);
    glVertex2f(fx0 + w*0.24f, fy0 + h*0.45f);
    glVertex2f(fx0 + w*0.16f, fy0 + h*0.60f);
    glVertex2f(fx0 + w*0.10f, fy0 + h*0.76f);
    glEnd();
    /* Nori — dark strip at bottom (trapezoidal) */
    glColor4f(0.10f, 0.18f, 0.08f, alpha);
    glBegin(GL_QUADS);
    glVertex2f(fx0+w*0.24f, fy0+h*0.60f); glVertex2f(fx0+w*0.76f, fy0+h*0.60f);
    glVertex2f(fx0+w*0.86f, fy0+h*0.90f); glVertex2f(fx0+w*0.14f, fy0+h*0.90f);
    glEnd();
    /* Nori texture lines */
    glColor4f(0.15f, 0.25f, 0.12f, alpha * 0.5f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(midX-w*0.14f, fy0+h*0.63f); glVertex2f(midX-w*0.16f, fy0+h*0.88f);
    glVertex2f(midX,         fy0+h*0.61f); glVertex2f(midX,         fy0+h*0.88f);
    glVertex2f(midX+w*0.14f, fy0+h*0.63f); glVertex2f(midX+w*0.16f, fy0+h*0.88f);
    glEnd();
    /* Outline */
    glColor4f(0.75f, 0.75f, 0.70f, alpha * 0.6f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(fx0 + w*0.08f, fy0 + h*0.90f);
    glVertex2f(fx0 + w*0.10f, fy0 + h*0.76f);
    glVertex2f(fx0 + w*0.16f, fy0 + h*0.60f);
    glVertex2f(fx0 + w*0.24f, fy0 + h*0.45f);
    glVertex2f(fx0 + w*0.34f, fy0 + h*0.30f);
    glVertex2f(fx0 + w*0.42f, fy0 + h*0.18f);
    glVertex2f(fx0 + w*0.48f, fy0 + h*0.13f);
    glVertex2f(midX,          fy0 + h*0.11f);
    glVertex2f(fx0 + w*0.52f, fy0 + h*0.13f);
    glVertex2f(fx0 + w*0.58f, fy0 + h*0.18f);
    glVertex2f(fx0 + w*0.66f, fy0 + h*0.30f);
    glVertex2f(fx0 + w*0.76f, fy0 + h*0.45f);
    glVertex2f(fx0 + w*0.84f, fy0 + h*0.60f);
    glVertex2f(fx0 + w*0.90f, fy0 + h*0.76f);
    glVertex2f(fx0 + w*0.92f, fy0 + h*0.90f);
    glEnd();
    /* Rice grain dots */
    glColor4f(0.90f, 0.90f, 0.86f, alpha * 0.5f);
    glPointSize(1.5f);
    glBegin(GL_POINTS);
    glVertex2f(midX-w*0.10f, fy0+h*0.30f);
    glVertex2f(midX+w*0.12f, fy0+h*0.36f);
    glVertex2f(midX-w*0.04f, fy0+h*0.22f);
    glVertex2f(midX+w*0.07f, fy0+h*0.46f);
    glVertex2f(midX,         fy0+h*0.28f);
    glEnd();
    glPointSize(1.0f);
}

static void renderAyamGoreng(float fx0, float fy0, float w, float h, float alpha) {
    float midX = fx0 + w * 0.5f;
    int k;
    /* Daging — bentuk pear/paha (lebar atas, menyempit ke bawah) */
    float cx = midX - w*0.02f, cy = fy0 + h*0.35f;
    glColor4f(0.85f, 0.60f, 0.20f, alpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (k = 0; k <= 20; k++) {
        float ang = (float)k * (2.0f * (float)M_PI / 20.0f);
        /* Pear shape: lebih lebar di atas, ramping di bawah */
        float rxBase = w * 0.36f;
        float ryBase = h * 0.28f;
        float stretch = 1.0f;
        if (sinf(ang) > 0) stretch = 0.70f; /* bawah lebih sempit */
        glVertex2f(cx + rxBase * cosf(ang), cy + ryBase * sinf(ang) * stretch);
    }
    glEnd();
    /* Kulit crispy — bintik gelap acak */
    glColor4f(0.72f, 0.42f, 0.10f, alpha * 0.6f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx-w*0.10f, cy-h*0.02f);
    for (k = 0; k <= 8; k++) {
        float ang = (float)k * (2.0f * (float)M_PI / 8.0f);
        glVertex2f(cx-w*0.10f + w*0.09f*cosf(ang), cy-h*0.02f + h*0.07f*sinf(ang));
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx+w*0.08f, cy-h*0.10f);
    for (k = 0; k <= 6; k++) {
        float ang = (float)k * (2.0f * (float)M_PI / 6.0f);
        glVertex2f(cx+w*0.08f + w*0.07f*cosf(ang), cy-h*0.10f + h*0.05f*sinf(ang));
    }
    glEnd();
    /* Highlight kilap daging */
    glColor4f(0.95f, 0.78f, 0.30f, alpha * 0.4f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx-w*0.06f, cy-h*0.14f);
    for (k = 0; k <= 8; k++) {
        float ang = (float)k * (2.0f * (float)M_PI / 8.0f);
        glVertex2f(cx-w*0.06f + w*0.10f*cosf(ang), cy-h*0.14f + h*0.05f*sinf(ang));
    }
    glEnd();
    /* Tulang — batang miring ke bawah-kanan */
    glColor4f(0.95f, 0.92f, 0.82f, alpha);
    glBegin(GL_QUADS);
    glVertex2f(midX+w*0.02f, fy0+h*0.52f); glVertex2f(midX+w*0.10f, fy0+h*0.50f);
    glVertex2f(midX+w*0.22f, fy0+h*0.88f); glVertex2f(midX+w*0.14f, fy0+h*0.90f);
    glEnd();
    /* Bulatan tulang bawah */
    {
        float bcx = midX+w*0.18f, bcy = fy0+h*0.90f, br = w*0.07f;
        glColor4f(0.93f, 0.90f, 0.80f, alpha);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(bcx, bcy);
        for (k = 0; k <= 10; k++) {
            float ang = (float)k * (2.0f * (float)M_PI / 10.0f);
            glVertex2f(bcx + br*cosf(ang), bcy + br*0.7f*sinf(ang));
        }
        glEnd();
    }
}

static void renderTelurCeplok(float fx0, float fy0, float w, float h, float alpha) {
    float midX = fx0 + w * 0.5f;
    float midY = fy0 + h * 0.50f;
    int k;
    /* Putih telur — bentuk organik tidak rata, sedikit lebar ke bawah */
    glColor4f(0.97f, 0.97f, 0.95f, alpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(midX, midY);
    for (k = 0; k <= 24; k++) {
        float ang = (float)k * (2.0f * (float)M_PI / 24.0f);
        /* Vary radius to make it look organic/irregular */
        float wobble = 1.0f + 0.12f * sinf(ang * 3.0f) + 0.08f * cosf(ang * 5.0f);
        float rxW = w * 0.44f * wobble;
        float ryW = h * 0.44f * wobble;
        /* Sedikit lebih lebar ke bawah */
        if (sinf(ang) > 0) ryW *= 1.15f;
        glVertex2f(midX + rxW * cosf(ang), midY + ryW * sinf(ang));
    }
    glEnd();
    /* Pinggiran putih telur tipis — edge yang lebih gelap */
    glColor4f(0.90f, 0.90f, 0.87f, alpha * 0.6f);
    glBegin(GL_LINE_LOOP);
    for (k = 0; k <= 24; k++) {
        float ang = (float)k * (2.0f * (float)M_PI / 24.0f);
        float wobble = 1.0f + 0.12f * sinf(ang * 3.0f) + 0.08f * cosf(ang * 5.0f);
        float rxW = w * 0.44f * wobble;
        float ryW = h * 0.44f * wobble;
        if (sinf(ang) > 0) ryW *= 1.15f;
        glVertex2f(midX + rxW * cosf(ang), midY + ryW * sinf(ang));
    }
    glEnd();
    /* Kuning telur — lingkaran kuning glossy */
    glColor4f(0.96f, 0.75f, 0.08f, alpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(midX, midY - h*0.03f);
    for (k = 0; k <= 16; k++) {
        float ang = (float)k * (2.0f * (float)M_PI / 16.0f);
        glVertex2f(midX + w*0.17f * cosf(ang), midY - h*0.03f + h*0.17f * sinf(ang));
    }
    glEnd();
    /* Shadow ring pada kuning telur */
    glColor4f(0.85f, 0.62f, 0.05f, alpha * 0.4f);
    glBegin(GL_LINE_LOOP);
    for (k = 0; k <= 16; k++) {
        float ang = (float)k * (2.0f * (float)M_PI / 16.0f);
        glVertex2f(midX + w*0.17f * cosf(ang), midY - h*0.03f + h*0.17f * sinf(ang));
    }
    glEnd();
    /* Highlight glossy pada kuning telur — titik terang */
    glColor4f(1.0f, 0.95f, 0.60f, alpha * 0.85f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(midX - w*0.05f, midY - h*0.10f);
    for (k = 0; k <= 8; k++) {
        float ang = (float)k * (2.0f * (float)M_PI / 8.0f);
        glVertex2f(midX - w*0.05f + w*0.05f * cosf(ang), midY - h*0.10f + h*0.05f * sinf(ang));
    }
    glEnd();
}

/* Render seluruh objek makanan aktif di peta menggunakan logika raycaster billboarding */
void renderFoodItems(Player *player) {
    int i;
    float det    = player->dirX * player->planeY - player->planeX * player->dirY;
    int pitchInt = (int)(player->pitch + player->jumpZ * 120.0f);
    int horizY   = SCREEN_H / 2 + pitchInt;
    if (fabsf(det) < 0.00001f) det = 0.00001f;
    for (i = 0; i < gNumFoodItems; i++) {
        FoodItem *fi = &gFoodItems[i];
        float dx, dy, tX, tY, fullH, floorY, fadeAlpha;
        int screenX, behindWall, col, spriteH, spriteW, sX0, sX1, sY0, sY1;
        float fx0, fx1, fy0, fy1, w, h;
        float rotSway;
        fadeAlpha = 1.0f;
        if (!fi->active) continue;
        dx = fi->x - player->x; dy = fi->y - player->y;
        tX = (player->dirX * dy - player->dirY * dx) / det;
        tY = (player->planeY * dx - player->planeX * dy) / det;
        if (tY <= 0.1f) continue;
        /* Horizontal sway using bobPhase as rotation angle */
        rotSway = sinf(fi->bobPhase) * 0.08f * tY;
        screenX = (int)((float)(SCREEN_W / 2) * (1.0f + tX / tY));
        screenX += (int)rotSway;
        if (fi->lifetime > FOOD_LIFETIME - 5.0f) {
            fadeAlpha = (FOOD_LIFETIME - fi->lifetime) / 5.0f;
            if (fadeAlpha < 0.0f) fadeAlpha = 0.0f;
        }
        /* No bob — item sits on floor */
        fullH   = (float)SCREEN_H * WALL_HEIGHT_SCALE / tY;
        floorY  = (float)horizY + fullH * 0.5f;
        spriteH = (int)(fullH * 0.40f);
        if (spriteH < 2) spriteH = 2;
        spriteW = spriteH;
        sY1 = (int)floorY;
        sY0 = sY1 - spriteH;
        sX0 = screenX - spriteW / 2;
        sX1 = screenX + spriteW / 2;
        if (sX1 < 0 || sX0 >= SCREEN_W || sY1 < 0 || sY0 >= SCREEN_H) continue;
        behindWall = 1;
        { int step = (spriteW > 16) ? spriteW / 6 : 1;
          for (col = sX0; col <= sX1; col += step)
              if (col >= 0 && col < SCREEN_W && zBuffer[col] >= tY * 0.90f) { behindWall = 0; break; } }
        if (behindWall) continue;
        fx0 = (float)(sX0 < 0 ? 0 : sX0);
        fx1 = (float)(sX1 >= SCREEN_W ? SCREEN_W-1 : sX1);
        fy0 = (float)(sY0 < 0 ? 0 : sY0);
        fy1 = (float)(sY1 >= SCREEN_H ? SCREEN_H-1 : sY1);
        w = fx1 - fx0; h = fy1 - fy0;
        if (w < 1.0f || h < 1.0f) continue;
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        switch (fi->type) {
            case FOOD_SUSU_KOTAK:  renderSusuKotak(fx0, fy0, w, h, fadeAlpha);  break;
            case FOOD_NASI_PUTIH:  renderNasiPutih(fx0, fy0, w, h, fadeAlpha);  break;
            case FOOD_AYAM_GORENG: renderAyamGoreng(fx0, fy0, w, h, fadeAlpha); break;
            case FOOD_TELUR_CEPLOK:renderTelurCeplok(fx0, fy0, w, h, fadeAlpha);break;
        }
        glDisable(GL_BLEND);
    }
}
