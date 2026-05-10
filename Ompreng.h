#ifndef OMPRENG_H
#define OMPRENG_H

/*
 * Ompreng.h — Ompreng MBG (Makan Bergizi Gratis) Item
 *
 * Model 3D nampan makan bergizi gratis (ompreng) berbahan stainless steel
 * dengan 5 kompartemen sesuai referensi:
 *
 *   ┌─────────────────────────────────────────┐
 *   │  [1] kecil   │  [2] sedang  │  [3] kecil│
 *   ├──────────────┴──────────────┴───────────┤
 *   │  [4] besar (panjang)  │  [5] bulat      │
 *   └─────────────────────────────────────────┘
 *
 *   Kompartemen:
 *     #1 — Kiri atas   : kecil persegi (lauk kering)
 *     #2 — Tengah atas : sedang persegi (lauk utama)
 *     #3 — Kanan atas  : kecil persegi (sayuran/buah)
 *     #4 — Kiri bawah  : persegi panjang besar (nasi)
 *     #5 — Kanan bawah : bulat (sup / kuah)
 *
 * Item Type   : ITEM_OMPRENG
 * Efek pickup : +50 HP + +20 Armor (makan siang lengkap!)
 * Drop chance : 20% dari musuh tipe DEMON
 * Animasi     : mengambang (bob) + rotasi Y lambat
 *
 * Depends (included sebelum ini di main.cpp):
 *   map.h, player.h, raycaster.h, weapon.h, enemy.h, item.h
 *
 * Cara pakai:
 *   1. #include "Ompreng.h"  (setelah item.h)
 *   2. Panggil omprengInit()     di dalam fungsi init
 *   3. Panggil omprengUpdate()   di dalam game loop
 *   4. Panggil renderOmprengItems() di dalam render pass
 */

#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ═══════════════════════════════════════════════════════════
 *  KONSTANTA
 * ═══════════════════════════════════════════════════════════ */
#define ITEM_OMPRENG        10          /* type ID unik, tidak tabrakan dengan item.h */
#define MAX_OMPRENG         16          /* maks ompreng aktif di map sekaligus */

#define OMPRENG_PICKUP_R    1.65f       /* jarak auto-pickup (sama dengan ITEM_RADIUS) */
#define OMPRENG_BOB_SPEED   1.6f        /* rad/s, lebih lambat supaya terasa "berat" */
#define OMPRENG_ROT_SPEED   45.0f       /* deg/s, rotasi pelan */
#define OMPRENG_LIFETIME    30.0f       /* detik sebelum despawn */

/* Warna stainless steel ompreng */
#define SS_R  0.78f
#define SS_G  0.78f
#define SS_B  0.80f

/* Warna highlight terang (refleksi) */
#define SS_BRIGHT_R  0.92f
#define SS_BRIGHT_G  0.92f
#define SS_BRIGHT_B  0.95f

/* Warna bayangan dalam kompartemen */
#define SS_DARK_R  0.55f
#define SS_DARK_G  0.55f
#define SS_DARK_B  0.58f

/* ═══════════════════════════════════════════════════════════
 *  STRUCT
 * ═══════════════════════════════════════════════════════════ */
typedef struct {
    float x, y;          /* posisi di map (unit map) */
    float rotAngle;       /* rotasi Y saat ini (derajat) */
    float bobPhase;       /* fase bobbing (radian) */
    float lifetime;       /* detik sudah ada */
    int   active;
} Ompreng;

/* ═══════════════════════════════════════════════════════════
 *  GLOBALS
 * ═══════════════════════════════════════════════════════════ */
static Ompreng gOmpreng[MAX_OMPRENG];
static int     gNumOmpreng = 0;

/* ═══════════════════════════════════════════════════════════
 *  INIT
 * ═══════════════════════════════════════════════════════════ */
static void omprengInit(void) {
    memset(gOmpreng, 0, sizeof(gOmpreng));
    gNumOmpreng = 0;
}

/* ═══════════════════════════════════════════════════════════
 *  SPAWN
 * ═══════════════════════════════════════════════════════════ */
static void omprengSpawn(float x, float y) {
    int i;
    for (i = 0; i < MAX_OMPRENG; i++) {
        if (!gOmpreng[i].active) {
            gOmpreng[i].x         = x;
            gOmpreng[i].y         = y;
            gOmpreng[i].rotAngle  = (float)(rand() % 360);
            gOmpreng[i].bobPhase  = (float)(rand() % 628) / 100.0f;
            gOmpreng[i].lifetime  = 0.0f;
            gOmpreng[i].active    = 1;
            if (i >= gNumOmpreng) gNumOmpreng = i + 1;
            return;
        }
    }
}

/* ═══════════════════════════════════════════════════════════
 *  DROP CHANCE — panggil saat musuh DEMON mati
 *  (tambahkan baris ini ke itemTryDrop atau enemyDeath callback)
 * ═══════════════════════════════════════════════════════════ */
static void omprengTryDrop(float x, float y, int enemyType) {
    /* 20% chance dari ENEMY_DEMON, 5% dari jenis lain */
    int threshold = (enemyType == 1 /* ENEMY_DEMON */) ? 20 : 5;
    if ((rand() % 100) < threshold) {
        omprengSpawn(x, y);
    }
}

/* ═══════════════════════════════════════════════════════════
 *  EFEK PICKUP
 * ═══════════════════════════════════════════════════════════ */
static void omprengApplyPickup(Player* player) {
    /* +50 HP (capped 100) */
    player->health += 50;
    if (player->health > 100) player->health = 100;

    /* +20 Armor (capped 100) */
    player->armor += 20;
    if (player->armor > 100) player->armor = 100;

    /* Bonus skor */
    /* gPlayerScore += 25; — uncomment jika pakai scoring dari item.h */
}

/* ═══════════════════════════════════════════════════════════
 *  UPDATE
 * ═══════════════════════════════════════════════════════════ */
static void omprengUpdate(Player* player, float dt) {
    int i;
    for (i = 0; i < gNumOmpreng; i++) {
        Ompreng* om = &gOmpreng[i];
        float dx, dy, dist;

        if (!om->active) continue;

        /* Animasi */
        om->rotAngle += OMPRENG_ROT_SPEED * dt;
        if (om->rotAngle >= 360.0f) om->rotAngle -= 360.0f;
        om->bobPhase += OMPRENG_BOB_SPEED * dt;
        om->lifetime += dt;

        /* Despawn */
        if (om->lifetime >= OMPRENG_LIFETIME) {
            om->active = 0;
            continue;
        }

        /* Pickup */
        dx   = player->x - om->x;
        dy   = player->y - om->y;
        dist = sqrtf(dx * dx + dy * dy);
        if (dist <= OMPRENG_PICKUP_R) {
            omprengApplyPickup(player);
            om->active = 0;
        }
    }
}

/* ═══════════════════════════════════════════════════════════
 *  MODEL 3D — RENDER SATU OMPRENG
 *
 *  Koordinat lokal (setelah glPushMatrix + rotasi Y + bob):
 *    X : kiri/kanan  (−W/2 … +W/2)
 *    Y : bawah/atas  (0 … H)
 *    Z : depan/belakang (−D/2 … +D/2)
 *
 *  Dimensi referensi (skala dunia game, 1 unit ≈ 1 cell MAP_SCALE=3):
 *    Lebar  (X) : 0.60
 *    Tinggi (Y) : 0.08
 *    Dalam  (Z) : 0.45
 * ═══════════════════════════════════════════════════════════ */

/* ── Helper: gambar quad datar (floor) untuk satu kompartemen ── */
static void omprengCompartmentFloor(
        float x0, float x1,   /* batas X lokal */
        float z0, float z1,   /* batas Z lokal */
        float y,              /* ketinggian Y */
        float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex3f(x0, y, z0);
        glVertex3f(x1, y, z0);
        glVertex3f(x1, y, z1);
        glVertex3f(x0, y, z1);
    glEnd();
}

/* ── Helper: gambar dinding tipis (divider) ── */
static void omprengDividerX(float x, float z0, float z1, float yBot, float yTop) {
    /* dinding paralel sumbu Z pada posisi X tertentu */
    glBegin(GL_QUADS);
        glVertex3f(x, yBot, z0);
        glVertex3f(x, yBot, z1);
        glVertex3f(x, yTop, z1);
        glVertex3f(x, yTop, z0);
    glEnd();
}

static void omprengDividerZ(float z, float x0, float x1, float yBot, float yTop) {
    /* dinding paralel sumbu X pada posisi Z tertentu */
    glBegin(GL_QUADS);
        glVertex3f(x0, yBot, z);
        glVertex3f(x1, yBot, z);
        glVertex3f(x1, yTop, z);
        glVertex3f(x0, yTop, z);
    glEnd();
}

/* ── Render bingkai luar (rim) ompreng ── */
static void omprengRenderFrame(void) {
    /* Dimensi badan utama */
    const float W2  =  0.300f;   /* setengah lebar */
    const float D2  =  0.225f;   /* setengah dalam */
    const float BOT =  0.000f;   /* Y dasar */
    const float TOP =  0.080f;   /* Y bibir */
    const float MID =  0.055f;   /* Y lantai kompartemen */
    const float TH  =  0.010f;   /* ketebalan dinding */

    /* ── Lantai utama (dasar nampan) ── */
    omprengCompartmentFloor(-W2, W2, -D2, D2, BOT, SS_DARK_R, SS_DARK_G, SS_DARK_B);

    /* ── Dinding luar — 4 sisi ── */
    glColor3f(SS_R, SS_G, SS_B);

    /* Sisi depan (Z negatif) */
    omprengDividerZ(-D2, -W2, W2, BOT, TOP);

    /* Sisi belakang (Z positif) */
    omprengDividerZ( D2, -W2, W2, BOT, TOP);

    /* Sisi kiri (X negatif) */
    omprengDividerX(-W2, -D2, D2, BOT, TOP);

    /* Sisi kanan (X positif) */
    omprengDividerX( W2, -D2, D2, BOT, TOP);

    /* ── Bibir atas / rim (strip tipis datar di puncak) ── */
    glColor3f(SS_BRIGHT_R, SS_BRIGHT_G, SS_BRIGHT_B);
    /* Depan */
    omprengCompartmentFloor(-W2-TH, W2+TH, -D2-TH, -D2, TOP, SS_BRIGHT_R, SS_BRIGHT_G, SS_BRIGHT_B);
    /* Belakang */
    omprengCompartmentFloor(-W2-TH, W2+TH,  D2, D2+TH, TOP, SS_BRIGHT_R, SS_BRIGHT_G, SS_BRIGHT_B);
    /* Kiri */
    omprengCompartmentFloor(-W2-TH, -W2, -D2, D2, TOP, SS_BRIGHT_R, SS_BRIGHT_G, SS_BRIGHT_B);
    /* Kanan */
    omprengCompartmentFloor(  W2, W2+TH, -D2, D2, TOP, SS_BRIGHT_R, SS_BRIGHT_G, SS_BRIGHT_B);

    /* ── Divider horizontal (batas baris atas/bawah) Z = 0 ── */
    glColor3f(SS_R, SS_G, SS_B);
    omprengDividerZ(0.0f, -W2, W2, BOT, MID + 0.008f);

    /* ── Divider vertikal baris atas ── */
    /* Baris atas: 3 sel — X divider di −0.10 dan +0.10 */
    omprengDividerX(-0.100f, -D2, 0.0f, BOT, MID + 0.008f);
    omprengDividerX( 0.100f, -D2, 0.0f, BOT, MID + 0.008f);

    /* ── Divider vertikal baris bawah ── */
    /* Baris bawah: 2 sel — X divider di +0.07 (kanan lebih kecil = bulat) */
    omprengDividerX( 0.070f, 0.0f, D2, BOT, MID + 0.008f);

    /* ── Lantai tiap kompartemen (sedikit lebih gelap) ── */
    /* Baris atas kiri */
    omprengCompartmentFloor(-W2+0.005f, -0.105f, -D2+0.005f, -0.005f, MID, SS_DARK_R, SS_DARK_G, SS_DARK_B);
    /* Baris atas tengah */
    omprengCompartmentFloor(-0.095f, 0.095f,  -D2+0.005f, -0.005f, MID, SS_DARK_R, SS_DARK_G, SS_DARK_B);
    /* Baris atas kanan */
    omprengCompartmentFloor( 0.105f, W2-0.005f, -D2+0.005f, -0.005f, MID, SS_DARK_R, SS_DARK_G, SS_DARK_B);
    /* Baris bawah kiri (nasi) */
    omprengCompartmentFloor(-W2+0.005f, 0.065f,  0.005f, D2-0.005f, MID, SS_DARK_R, SS_DARK_G, SS_DARK_B);
    /* Baris bawah kanan (sup — lebih gelap sedikit) */
    omprengCompartmentFloor( 0.075f, W2-0.005f,  0.005f, D2-0.005f, MID, SS_DARK_R*0.9f, SS_DARK_G*0.9f, SS_DARK_B*0.9f);

    (void)MID; /* suppress unused warning */
    (void)TH;
}

/* ── Render kompartemen #5 bulat (pakai gluDisk via gQuad) ── */
static void omprengRenderRoundComp(void) {
    if (!gQuad) return;

    /* Lingkaran lantai dalam kompartemen bulat */
    glColor3f(SS_DARK_R * 0.85f, SS_DARK_G * 0.85f, SS_DARK_B * 0.85f);
    glPushMatrix();
        glTranslatef(0.185f, 0.055f, 0.113f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);  /* putar agar hadap atas */
        gluDisk(gQuad, 0.0f, 0.095f, 24, 1);
    glPopMatrix();

    /* Ring batas lingkaran — highlight */
    glColor3f(SS_R, SS_G, SS_B);
    glPushMatrix();
        glTranslatef(0.185f, 0.055f, 0.113f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluDisk(gQuad, 0.093f, 0.103f, 24, 1);
    glPopMatrix();
}

/* ── Efek highlight refleksi diagonal (goresan stainless) ── */
static void omprengRenderHighlights(void) {
    glColor3f(SS_BRIGHT_R, SS_BRIGHT_G, SS_BRIGHT_B);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
        /* goresan diagonal kiri */
        glVertex3f(-0.25f, 0.082f, -0.20f);
        glVertex3f(-0.10f, 0.082f, -0.05f);
        /* goresan diagonal tengah */
        glVertex3f( 0.00f, 0.082f, -0.18f);
        glVertex3f( 0.10f, 0.082f, -0.10f);
        /* goresan kanan bawah */
        glVertex3f( 0.10f, 0.082f,  0.05f);
        glVertex3f( 0.25f, 0.082f,  0.18f);
    glEnd();
    glLineWidth(1.0f);
}

/* ── Fungsi utama render model 3D ompreng ── */
static void renderOmprengModel(float bob) {
    glPushMatrix();
        /* Naikkan sedikit dari lantai + efek bob */
        glTranslatef(0.0f, 0.12f + bob, 0.0f);

        /* Bingkai + divider + lantai kompartemen */
        omprengRenderFrame();

        /* Kompartemen bulat kanan bawah */
        omprengRenderRoundComp();

        /* Goresan refleksi stainless */
        omprengRenderHighlights();

    glPopMatrix();
}

/* ═══════════════════════════════════════════════════════════
 *  RENDER PASS — tampilkan semua ompreng aktif sebagai sprite 2D
 *  (Pola sama persis dengan renderItems() di item.h)
 * ═══════════════════════════════════════════════════════════ */
static void renderOmprengItems(Player* player) {
    int   i;
    float det      = player->dirX * player->planeY - player->planeX * player->dirY;
    int   pitchInt = (int)player->pitch;
    int   horizY   = SCREEN_H / 2 + pitchInt;

    if (fabsf(det) < 0.00001f) det = 0.00001f;

    for (i = 0; i < gNumOmpreng; i++) {
        Ompreng* om = &gOmpreng[i];
        float dx, dy, tX, tY;
        int   screenX, behindWall, col;
        float fullH, floorY, bob;
        int   spriteH, spriteW, sX0, sX1, sY0, sY1;
        float fx0, fx1, fy0, fy1, midX, w, h;
        float fadeAlpha = 1.0f;

        if (!om->active) continue;

        dx = om->x - player->x;
        dy = om->y - player->y;

        tX = (player->dirX * dy - player->dirY * dx) / det;
        tY = (player->planeY * dx - player->planeX * dy) / det;
        if (tY <= 0.1f) continue;

        screenX = (int)((float)(SCREEN_W / 2) * (1.0f + tX / tY));

        /* Fade 5 detik terakhir sebelum despawn */
        if (om->lifetime > OMPRENG_LIFETIME - 5.0f) {
            fadeAlpha = (OMPRENG_LIFETIME - om->lifetime) / 5.0f;
            if (fadeAlpha < 0.0f) fadeAlpha = 0.0f;
        }

        bob     = sinf(om->bobPhase) * 0.06f * tY;
        fullH   = (float)SCREEN_H * WALL_HEIGHT_SCALE / tY;
        floorY  = (float)horizY + fullH * 0.5f;

        spriteH = (int)(fullH * 0.45f);   /* ompreng agak lebih besar dari item biasa */
        if (spriteH < 2) spriteH = 2;
        spriteW = (int)(spriteH * 1.35f); /* nampan lebih lebar dari tinggi */

        sY1 = (int)floorY - (int)(fullH * bob);
        sY0 = sY1 - spriteH;
        sX0 = screenX - spriteW / 2;
        sX1 = screenX + spriteW / 2;
        if (sX1 < 0 || sX0 >= SCREEN_W || sY1 < 0 || sY0 >= SCREEN_H) continue;

        /* Oklusi dinding */
        behindWall = 1;
        { int step = (spriteW > 16) ? spriteW / 6 : 1;
          for (col = sX0; col <= sX1; col += step)
              if (col >= 0 && col < SCREEN_W && zBuffer[col] >= tY * 0.90f)
                  { behindWall = 0; break; } }
        if (behindWall) continue;

        /* Clamp rect gambar */
        fx0  = (float)(sX0 < 0 ? 0 : sX0);
        fx1  = (float)(sX1 >= SCREEN_W ? SCREEN_W - 1 : sX1);
        fy0  = (float)(sY0 < 0 ? 0 : sY0);
        fy1  = (float)(sY1 >= SCREEN_H ? SCREEN_H - 1 : sY1);
        midX = (fx0 + fx1) * 0.5f;
        w    = fx1 - fx0;
        h    = fy1 - fy0;
        if (w < 1.0f || h < 1.0f) continue;

        if (fadeAlpha < 1.0f) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        /* ── Gambar sprite 2D ompreng (top-down silhouette) ── */

        /* Badan utama — stainless terang */
        glColor4f(SS_R, SS_G, SS_B, fadeAlpha);
        glBegin(GL_QUADS);
            glVertex2f(fx0,          fy0 + h * 0.12f);
            glVertex2f(fx1,          fy0 + h * 0.12f);
            glVertex2f(fx1,          fy1);
            glVertex2f(fx0,          fy1);
        glEnd();

        /* Bibir atas — highlight terang */
        glColor4f(SS_BRIGHT_R, SS_BRIGHT_G, SS_BRIGHT_B, fadeAlpha);
        glBegin(GL_QUADS);
            glVertex2f(fx0,          fy0);
            glVertex2f(fx1,          fy0);
            glVertex2f(fx1,          fy0 + h * 0.12f);
            glVertex2f(fx0,          fy0 + h * 0.12f);
        glEnd();

        /* Divider horizontal baris tengah */
        glColor4f(SS_DARK_R, SS_DARK_G, SS_DARK_B, fadeAlpha);
        glBegin(GL_QUADS);
            glVertex2f(fx0,          fy0 + h * 0.50f);
            glVertex2f(fx1,          fy0 + h * 0.50f);
            glVertex2f(fx1,          fy0 + h * 0.54f);
            glVertex2f(fx0,          fy0 + h * 0.54f);
        glEnd();

        /* Divider vertikal baris atas — kiri */
        glBegin(GL_QUADS);
            glVertex2f(midX - w*0.33f, fy0 + h * 0.12f);
            glVertex2f(midX - w*0.30f, fy0 + h * 0.12f);
            glVertex2f(midX - w*0.30f, fy0 + h * 0.50f);
            glVertex2f(midX - w*0.33f, fy0 + h * 0.50f);
        glEnd();

        /* Divider vertikal baris atas — kanan */
        glBegin(GL_QUADS);
            glVertex2f(midX + w*0.30f, fy0 + h * 0.12f);
            glVertex2f(midX + w*0.33f, fy0 + h * 0.12f);
            glVertex2f(midX + w*0.33f, fy0 + h * 0.50f);
            glVertex2f(midX + w*0.30f, fy0 + h * 0.50f);
        glEnd();

        /* Divider vertikal baris bawah */
        glBegin(GL_QUADS);
            glVertex2f(midX + w*0.12f, fy0 + h * 0.54f);
            glVertex2f(midX + w*0.15f, fy0 + h * 0.54f);
            glVertex2f(midX + w*0.15f, fy1);
            glVertex2f(midX + w*0.12f, fy1);
        glEnd();

        /* Lingkaran kompartemen kanan bawah */
        /* (approximate pakai oktagon) */
        {
            float cx = midX + w * 0.31f;
            float cy = fy0  + h * 0.77f;
            float cr = h   * 0.20f;
            int   k;
            glColor4f(SS_DARK_R*0.88f, SS_DARK_G*0.88f, SS_DARK_B*0.88f, fadeAlpha);
            glBegin(GL_TRIANGLE_FAN);
                glVertex2f(cx, cy);
                for (k = 0; k <= 12; k++) {
                    float ang = (float)k * (2.0f * (float)M_PI / 12.0f);
                    glVertex2f(cx + cr * cosf(ang) * 0.85f,
                               cy + cr * sinf(ang));
                }
            glEnd();
        }

        /* Kilap / highlight diagonal */
        glColor4f(SS_BRIGHT_R, SS_BRIGHT_G, SS_BRIGHT_B, fadeAlpha * 0.6f);
        glLineWidth(1.5f);
        glBegin(GL_LINES);
            glVertex2f(fx0 + w * 0.10f, fy0 + h * 0.20f);
            glVertex2f(fx0 + w * 0.25f, fy0 + h * 0.35f);
        glEnd();
        glLineWidth(1.0f);

        if (fadeAlpha < 1.0f) glDisable(GL_BLEND);
    }
}

#endif /* OMPRENG_H */
