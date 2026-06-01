
#include "hud.h"
#include "enemy.h"
#include "weapon.h"
#include "raycaster.h"
#include "map.h"
#include "FoodItem.h"
#include "Ompreng.h"
#include "audio.h"
#include <GL/glut.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

extern int gGameWon;
extern int gShowControls;

static void drawText(float x, float y, const char* txt, void* font) {
    const char* c;
    glRasterPos2f(x, y);
    for (c = txt; *c != '\0'; c++) glutBitmapCharacter(font, *c);
}

static int textWidth(const char* txt, void* font) {
    int w = 0;
    const char* c;
    for (c = txt; *c != '\0'; c++) w += glutBitmapWidth(font, *c);
    return w;
}

static float getCenteredTextX(float logicalCenter, int tw) {
    int ww = glutGet(GLUT_WINDOW_WIDTH);
    if (ww <= 0) ww = SCREEN_W;
    return logicalCenter - ((float)tw * SCREEN_W / (2.0f * ww));
}

static void fillRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
        glVertex2f(x,   y);     glVertex2f(x+w, y);
        glVertex2f(x+w, y+h);   glVertex2f(x,   y+h);
    glEnd();
}

static void strokeRect(float x, float y, float w, float h) {
    glBegin(GL_LINE_LOOP);
        glVertex2f(x,   y);     glVertex2f(x+w, y);
        glVertex2f(x+w, y+h);   glVertex2f(x,   y+h);
    glEnd();
}

static void drawCrosshair(void) {
    float cx = SCREEN_W / 2.0f, cy = SCREEN_H / 2.0f;
    float sz = 10.0f, gap = 4.0f;
    glLineWidth(2.0f);
    glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
    glBegin(GL_LINES);
        glVertex2f(cx-sz-1, cy+1); glVertex2f(cx-gap-1, cy+1);
        glVertex2f(cx+gap+1, cy+1); glVertex2f(cx+sz+1, cy+1);
        glVertex2f(cx+1, cy-sz-1); glVertex2f(cx+1, cy-gap-1);
        glVertex2f(cx+1, cy+gap+1); glVertex2f(cx+1, cy+sz+1);
    glEnd();
    glColor3f(0.05f, 0.95f, 0.35f);
    glBegin(GL_LINES);
        glVertex2f(cx-sz, cy); glVertex2f(cx-gap, cy);
        glVertex2f(cx+gap, cy); glVertex2f(cx+sz, cy);
        glVertex2f(cx, cy-sz); glVertex2f(cx, cy-gap);
        glVertex2f(cx, cy+gap); glVertex2f(cx, cy+sz);
    glEnd();
    glPointSize(2.5f); glBegin(GL_POINTS); glVertex2f(cx, cy); glEnd();
    glLineWidth(1.0f); glPointSize(1.0f);
}

static void drawStatusBarBG(void) {
    float bh = 52.0f;
    glBegin(GL_QUADS);
        glColor4f(0.05f, 0.05f, 0.07f, 0.88f);
        glVertex2f(0, SCREEN_H - bh); glVertex2f(SCREEN_W, SCREEN_H - bh);
        glColor4f(0.02f, 0.02f, 0.04f, 0.95f);
        glVertex2f(SCREEN_W, SCREEN_H); glVertex2f(0, SCREEN_H);
    glEnd();
    glColor4f(0.25f, 0.60f, 1.0f, 0.50f);
    glLineWidth(1.5f);
    glBegin(GL_LINES); glVertex2f(0, SCREEN_H-bh); glVertex2f(SCREEN_W, SCREEN_H-bh); glEnd();
    glLineWidth(1.0f);
}

static void drawHealthBar(int health) {
    float bx = 20.0f, by = SCREEN_H - 38.0f, bw = 190.0f, bh = 18.0f;
    float r  = (float)health / 100.0f;
    char  txt[32]; int seg;
    if (r < 0.0f) r = 0.0f; if (r > 1.0f) r = 1.0f;
    glColor4f(0.08f, 0.08f, 0.08f, 0.90f); fillRect(bx-2, by-2, bw+4, bh+4);
    if (r > 0.5f)       glColor3f(0.08f, 0.82f, 0.22f);
    else if (r > 0.25f) glColor3f(0.90f, 0.72f, 0.08f);
    else                glColor3f(0.90f, 0.12f, 0.10f);
    fillRect(bx, by, bw*r, bh);
    glColor4f(0.0f, 0.0f, 0.0f, 0.40f);
    for (seg = 1; seg < 10; seg++) {
        float sx = bx + bw * (float)seg / 10.0f;
        glBegin(GL_LINES); glVertex2f(sx, by); glVertex2f(sx, by+bh); glEnd();
    }
    glColor4f(0.55f, 0.55f, 0.60f, 0.90f); strokeRect(bx, by, bw, bh);
    glColor3f(0.95f, 0.95f, 0.95f);
    sprintf(txt, "HP %d", health); drawText(bx+6, by+13, txt, GLUT_BITMAP_HELVETICA_12);
}

static void drawArmorBar(int armor) {
    float bx = 20.0f, by = SCREEN_H - 62.0f, bw = 190.0f, bh = 14.0f;
    float r  = (float)armor / 100.0f;
    char  txt[16];
    if (r < 0.0f) r = 0.0f; if (r > 1.0f) r = 1.0f;
    glColor4f(0.08f, 0.08f, 0.08f, 0.85f); fillRect(bx-2, by-2, bw+4, bh+4);
    glColor3f(0.18f, 0.58f, 0.90f); fillRect(bx, by, bw*r, bh);
    glColor4f(0.45f, 0.45f, 0.50f, 0.80f); strokeRect(bx, by, bw, bh);
    glColor3f(0.75f, 0.85f, 0.95f);
    sprintf(txt, "ARM %d", armor); drawText(bx+6, by+10, txt, GLUT_BITMAP_HELVETICA_12);
}

static void drawArenaPanel(void) {
    int ai;
    float py = 14.0f;
    for (ai = 0; ai < 3; ai++) {
        ArenaRoom* a = &gArenas[ai];
        char header[24];
        float bw = 200.0f, bh = 54.0f;
        float px = (float)(SCREEN_W / 2) - bw * 0.5f;
        if (a->state == ARENA_INACTIVE) continue;
        if (a->statusMsg[0] == '\0')    continue;
        sprintf(header, ai == 0 ? "TOP ROOM" : ai == 1 ? "BOTTOM ROOM" : "BOSS ROOM");
        glColor4f(0.04f, 0.04f, 0.08f, 0.82f); fillRect(px-2, py-2, bw+4, bh+4);
        if (a->state == ARENA_COMPLETE)      glColor4f(0.20f, 0.90f, 0.30f, 0.80f);
        else if (a->state == ARENA_COOLDOWN) glColor4f(0.90f, 0.70f, 0.10f, 0.80f);
        else                                 glColor4f(0.90f, 0.15f, 0.10f, 0.80f);
        strokeRect(px-2, py-2, bw+4, bh+4);
        glColor3f(0.70f, 0.70f, 0.75f);
        { int tw = textWidth(header, GLUT_BITMAP_HELVETICA_12); drawText(getCenteredTextX(px+bw*0.5f, tw), py+14, header, GLUT_BITMAP_HELVETICA_12); }
        if (a->state == ARENA_COMPLETE)      glColor3f(0.20f, 1.0f, 0.35f);
        else if (a->state == ARENA_COOLDOWN) glColor3f(1.0f, 0.85f, 0.15f);
        else {
            double t = (double)glutGet(GLUT_ELAPSED_TIME) / 1000.0;
            float  p = 0.75f + 0.25f * (float)(sin(t * 3.5));
            glColor3f(1.0f*p, 0.20f, 0.10f);
        }
        { int tw = textWidth(a->statusMsg, GLUT_BITMAP_HELVETICA_18); drawText(getCenteredTextX(px+bw*0.5f, tw), py+40, a->statusMsg, GLUT_BITMAP_HELVETICA_18); }
        if (a->state == ARENA_ACTIVE || a->state == ARENA_COOLDOWN) {
            float barW = bw - 12.0f;
            float prog = (float)(a->currentWave) / (float)ARENA_TOTAL_WAVES;
            float barX = px+6.0f, barY = py+46.0f, barH = 5.0f;
            glColor4f(0.15f, 0.15f, 0.18f, 0.80f); fillRect(barX, barY, barW, barH);
            glColor3f(0.90f, 0.30f, 0.10f); fillRect(barX, barY, barW*prog, barH);
        }
        py += bh + 10.0f;
    }
}

static void drawAmmoPanel(void) {
    int   ammo    = weaponGetAmmo();
    int   maxAmmo = weaponGetMaxAmmo();
    float ready   = weaponGetReadyRatio();
    float reload  = weaponGetReloadRatio();
    int   isRel   = weaponIsReloading();
    const char* wname = weaponGetName();
    float panW = 220.0f, panH = 38.0f;
    float px   = SCREEN_W - panW - 16.0f, py = SCREEN_H - panH - 7.0f;
    float ar   = (maxAmmo > 0) ? (float)ammo / (float)maxAmmo : 0.0f;
    char  txt[32];
    double t = (double)glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    if (gCurrentWeapon == WEAPON_NONE) {
        /* No weapon — show pick-up hint bottom-right */
        float pulse = 0.60f + 0.40f * (float)sin(t * 2.5);
        const char* hint = "Find a weapon!";
        int tw = textWidth(hint, GLUT_BITMAP_HELVETICA_18);
        float hx = SCREEN_W - (float)tw * SCREEN_W / (float)(glutGet(GLUT_WINDOW_WIDTH) > 0 ? glutGet(GLUT_WINDOW_WIDTH) : SCREEN_W) - 18.0f;
        glColor4f(0.85f * pulse, 0.75f * pulse, 0.10f * pulse, 0.90f);
        drawText(hx, SCREEN_H - 18.0f, hint, GLUT_BITMAP_HELVETICA_18);
        return;
    }
    glColor4f(0.08f, 0.08f, 0.08f, 0.90f); fillRect(px-2, py-2, panW+4, panH+4);
    if (isRel) {
        glColor3f(0.85f, 0.55f, 0.05f); fillRect(px, py, (panW-4)*reload, panH*0.45f);
    } else {
        if (ar > 0.5f)       glColor3f(0.15f, 0.55f, 0.90f);
        else if (ar > 0.25f) glColor3f(0.85f, 0.65f, 0.10f);
        else                 glColor3f(0.90f, 0.15f, 0.10f);
        fillRect(px, py, (panW-4)*ar, panH*0.45f);
        glColor4f(0.05f, 0.90f, 0.40f, 0.65f*ready); fillRect(px, py+panH*0.50f, (panW-4)*ready, panH*0.12f);
    }
    glColor4f(0.45f, 0.45f, 0.50f, 0.90f); strokeRect(px, py, panW, panH);
    glColor3f(0.85f, 0.85f, 0.90f); drawText(px+6, py+14, wname, GLUT_BITMAP_HELVETICA_12);
    if (isRel) {
        float pulse = 0.65f + 0.35f * (float)(sin(t * 6.0));
        glColor3f(1.0f*pulse, 0.70f*pulse, 0.10f*pulse);
        drawText(px+6, py+30, "RELOADING...", GLUT_BITMAP_HELVETICA_18);
    } else {
        sprintf(txt, "%d / %d", ammo, maxAmmo);
        glColor3f(0.95f, 0.90f, 0.50f); drawText(px+6, py+30, txt, GLUT_BITMAP_HELVETICA_18);
    }
}

static void drawWeaponSelector(void) {
    float iW = 62.0f, iH = 24.0f;
    float iY = SCREEN_H - iH - 6.0f;
    const char* names[NUM_WEAPONS] = {"1:PISTOL", "2:SHOTGUN", "3:M416"};
    int i, col;
    int numUnlocked = 0;
    if (gCurrentWeapon == WEAPON_NONE) return;  /* hide when unarmed */
    /* Count unlocked weapons to center the selector */
    for (i = 0; i < NUM_WEAPONS; i++)
        if (gWeapons[i].unlocked) numUnlocked++;
    if (numUnlocked == 0) return;
    float totalW = numUnlocked * iW + (numUnlocked - 1) * 4.0f;
    float sX = SCREEN_W / 2.0f - totalW / 2.0f;
    col = 0;
    for (i = 0; i < NUM_WEAPONS; i++) {
        float ix;
        int   act;
        if (!gWeapons[i].unlocked) continue;  /* skip locked weapons */
        ix  = sX + col * (iW + 4.0f);
        act = (i == gCurrentWeapon);
        glColor4f(act?0.15f:0.08f, act?0.45f:0.08f, act?0.80f:0.10f, act?0.88f:0.75f);
        fillRect(ix, iY, iW, iH);
        glColor4f(act?0.40f:0.35f, act?0.75f:0.35f, act?1.00f:0.40f, 1.0f);
        strokeRect(ix, iY, iW, iH);
        glColor3f(act?1.0f:0.60f, act?1.0f:0.60f, act?1.0f:0.65f);
        { int tw = textWidth(names[i], GLUT_BITMAP_HELVETICA_12); drawText(getCenteredTextX(ix+iW*0.5f, tw), iY+15, names[i], GLUT_BITMAP_HELVETICA_12); }
        col++;
    }
}

static void drawKillCounter(void) {
    char txt[64];
    int  kills = enemyGetKillCount();
    int  alive = enemyGetAliveCount();
    float px = 20.0f, py = 40.0f, bw = 160.0f, bh = 36.0f;
    glColor4f(0.05f, 0.05f, 0.05f, 0.72f); fillRect(px-2, py-2, bw+4, bh+4);
    glColor4f(0.70f, 0.25f, 0.10f, 0.70f); strokeRect(px-2, py-2, bw+4, bh+4);
    glColor3f(0.90f, 0.75f, 0.20f);
    sprintf(txt, "KILLS: %d", kills); drawText(px+6, py+14, txt, GLUT_BITMAP_HELVETICA_12);
    if (alive > 0) glColor3f(0.90f, 0.30f, 0.20f);
    else           glColor3f(0.20f, 0.90f, 0.30f);
    sprintf(txt, alive > 0 ? "ENEMIES: %d" : "ALL CLEAR!", alive);
    drawText(px+6, py+28, txt, GLUT_BITMAP_HELVETICA_12);
}

static void drawMinimap(Player* player) {
    float mSz = 140.0f;
    float mx  = SCREEN_W - mSz - 14.0f, my = 14.0f;
    float cw  = mSz / MAP_LOGICAL_W, ch = mSz / MAP_LOGICAL_H;
    float invScale = 1.0f / MAP_SCALE;
    int   x, y, i;
    float px, py;
    glColor4f(0.0f, 0.0f, 0.0f, 0.55f); fillRect(mx-2, my-2, mSz+4, mSz+4);
    for (y = 0; y < MAP_LOGICAL_H; y++) {
        for (x = 0; x < MAP_LOGICAL_W; x++) {
            int wt = worldMap[y][x];
            if (wt > 0) {
                if (wt >= numWallColors) wt = 1;
                glColor3f(wallColors[wt][0]*0.75f, wallColors[wt][1]*0.75f, wallColors[wt][2]*0.75f);
                fillRect(mx + x*cw, my + y*ch, cw, ch);
            }
        }
    }
    for (i = 0; i < gNumEnemies; i++) {
        Enemy* e = &gEnemies[i];
        if (!e->active) continue;
        switch (e->type) {
            case ENEMY_IMP:     glColor3f(0.90f, 0.15f, 0.10f); break;
            case ENEMY_DEMON:   glColor3f(0.15f, 0.65f, 0.15f); break;
            case ENEMY_SPECTRE: glColor3f(0.20f, 0.60f, 0.95f); break;
        }
        glPointSize(4.0f);
        glBegin(GL_POINTS); glVertex2f(mx + e->x*invScale*cw, my + e->y*invScale*ch); glEnd();
    }
    px = mx + player->x * invScale * cw;
    py = my + player->y * invScale * ch;
    glColor3f(0.10f, 0.95f, 0.30f);
    glLineWidth(1.5f);
    glBegin(GL_LINES); glVertex2f(px, py); glVertex2f(px + player->dirX*10.0f, py + player->dirY*10.0f); glEnd();
    glLineWidth(1.0f);
    glPointSize(5.5f); glBegin(GL_POINTS); glVertex2f(px, py); glEnd(); glPointSize(1.0f);
    glColor4f(0.30f, 0.55f, 0.80f, 0.70f); strokeRect(mx-2, my-2, mSz+4, mSz+4);
}

static void drawDamageFlash(void) {
    if (gDamageFlash <= 0.0f) return;
    float a = gDamageFlash * 0.45f;
    glColor4f(0.85f, 0.05f, 0.05f, a); fillRect(0, 0, SCREEN_W, SCREEN_H);
}

static void drawItemCollectionPanel(void) {
    const char* names[5] = {"Susu Kotak", "Nasi Putih", "Ayam Goreng", "Telur Ceplok", "Ompreng"};
    int collected[5];
    int i, total = 0;
    float px = 20.0f, py = 90.0f, bw = 160.0f, lineH = 14.0f;
    float bh = 22.0f + 5 * lineH + 18.0f;
    char txt[32];
    collected[0] = gFoodCollected[FOOD_SUSU_KOTAK];
    collected[1] = gFoodCollected[FOOD_NASI_PUTIH];
    collected[2] = gFoodCollected[FOOD_AYAM_GORENG];
    collected[3] = gFoodCollected[FOOD_TELUR_CEPLOK];
    collected[4] = gOmprengCollected;
    for (i = 0; i < 5; i++) if (collected[i]) total++;
    glColor4f(0.05f, 0.05f, 0.05f, 0.72f); fillRect(px-2, py-2, bw+4, bh+4);
    glColor4f(0.60f, 0.40f, 0.10f, 0.70f); strokeRect(px-2, py-2, bw+4, bh+4);
    glColor3f(0.95f, 0.85f, 0.40f);
    drawText(px+6, py+14, "ITEMS", GLUT_BITMAP_HELVETICA_12);
    for (i = 0; i < 5; i++) {
        float iy = py + 22.0f + i * lineH;
        if (collected[i]) {
            glColor3f(0.20f, 0.90f, 0.30f);
            sprintf(txt, "[v] %s", names[i]);
        } else {
            glColor3f(0.50f, 0.50f, 0.55f);
            sprintf(txt, "[ ] %s", names[i]);
        }
        drawText(px+10, iy+10, txt, GLUT_BITMAP_HELVETICA_12);
    }
    /* Progress bar */
    {
        float barX = px+6, barY = py + bh - 14.0f, barW = bw - 12.0f, barH = 8.0f;
        float prog = (float)total / 5.0f;
        glColor4f(0.15f, 0.15f, 0.18f, 0.80f); fillRect(barX, barY, barW, barH);
        if (total == 5) glColor3f(0.20f, 0.95f, 0.30f);
        else            glColor3f(0.90f, 0.70f, 0.10f);
        fillRect(barX, barY, barW * prog, barH);
        glColor3f(0.80f, 0.80f, 0.85f);
        sprintf(txt, "%d/5", total);
        drawText(barX + barW * 0.5f - 8, barY + 7, txt, GLUT_BITMAP_HELVETICA_10);
    }
}

static void drawGameOver(void) {
    glColor4f(0.0f, 0.0f, 0.0f, 0.78f); fillRect(0, 0, SCREEN_W, SCREEN_H);
    {
        const char* msg = "YOU DIED";
        int tw = textWidth(msg, GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.85f, 0.08f, 0.08f);
        drawText(getCenteredTextX(SCREEN_W/2.0f, tw), SCREEN_H/2.0f-20, msg, GLUT_BITMAP_TIMES_ROMAN_24);
    }
    {
        const char* sub = "Press F5 to restart";
        int tw = textWidth(sub, GLUT_BITMAP_HELVETICA_18);
        glColor3f(0.65f, 0.65f, 0.65f);
        drawText(getCenteredTextX(SCREEN_W/2.0f, tw), SCREEN_H/2.0f+18, sub, GLUT_BITMAP_HELVETICA_18);
    }
}

static void drawVictoryScreen(void) {
    float bw = 320.0f, bh = 160.0f;
    float bx = (SCREEN_W - bw) / 2.0f, by = (SCREEN_H - bh) / 2.0f;
    double t = (double)glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    float pulse = 0.85f + 0.15f * (float)sin(t * 2.0);
    /* Dark overlay */
    glColor4f(0.0f, 0.0f, 0.0f, 0.75f); fillRect(0, 0, SCREEN_W, SCREEN_H);
    /* Victory box */
    glColor4f(0.08f, 0.12f, 0.08f, 0.92f); fillRect(bx, by, bw, bh);
    glColor4f(0.30f, 0.95f, 0.40f, 0.90f); strokeRect(bx, by, bw, bh);
    glColor4f(0.25f, 0.85f, 0.35f, 0.60f); strokeRect(bx+3, by+3, bw-6, bh-6);
    /* Title */
    {
        const char* msg = "VICTORY!";
        int tw = textWidth(msg, GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.30f * pulse, 1.0f * pulse, 0.40f * pulse);
        drawText(getCenteredTextX(SCREEN_W/2.0f, tw), by + 40, msg, GLUT_BITMAP_TIMES_ROMAN_24);
    }
    /* Subtitle */
    {
        const char* sub1 = "All items collected!";
        const char* sub2 = "Boss defeated!";
        int tw1 = textWidth(sub1, GLUT_BITMAP_HELVETICA_18);
        int tw2 = textWidth(sub2, GLUT_BITMAP_HELVETICA_18);
        glColor3f(0.85f, 0.90f, 0.85f);
        drawText(getCenteredTextX(SCREEN_W/2.0f, tw1), by + 72, sub1, GLUT_BITMAP_HELVETICA_18);
        drawText(getCenteredTextX(SCREEN_W/2.0f, tw2), by + 95, sub2, GLUT_BITMAP_HELVETICA_18);
    }
    /* Controls */
    {
        const char* ctrl = "Press ESC to quit | F5 to replay";
        int tw = textWidth(ctrl, GLUT_BITMAP_HELVETICA_12);
        glColor3f(0.60f, 0.65f, 0.60f);
        drawText(getCenteredTextX(SCREEN_W/2.0f, tw), by + 130, ctrl, GLUT_BITMAP_HELVETICA_12);
    }
    /* Kill count */
    {
        char buf[48];
        int tw;
        sprintf(buf, "Total Kills: %d", enemyGetKillCount());
        tw = textWidth(buf, GLUT_BITMAP_HELVETICA_12);
        glColor3f(0.90f, 0.80f, 0.30f);
        drawText(getCenteredTextX(SCREEN_W/2.0f, tw), by + 148, buf, GLUT_BITMAP_HELVETICA_12);
    }
}

static void drawControlsPopup(void) {
    float bw = 360.0f, bh = 310.0f;
    float bx = (SCREEN_W - bw) / 2.0f, by = (SCREEN_H - bh) / 2.0f;
    double t = (double)glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    float pulse = 0.85f + 0.15f * (float)sin(t * 2.0);
    float lineY;
    /* Dark overlay */
    glColor4f(0.0f, 0.0f, 0.0f, 0.80f); fillRect(0, 0, SCREEN_W, SCREEN_H);
    /* Green box */
    glColor4f(0.06f, 0.14f, 0.06f, 0.94f); fillRect(bx, by, bw, bh);
    glColor4f(0.30f, 0.95f, 0.40f, 0.90f); strokeRect(bx, by, bw, bh);
    glColor4f(0.25f, 0.85f, 0.35f, 0.60f); strokeRect(bx+3, by+3, bw-6, bh-6);
    /* Title */
    {
        const char* msg = "=== CONTROLS ===";
        int tw = textWidth(msg, GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.30f * pulse, 1.0f * pulse, 0.40f * pulse);
        drawText(getCenteredTextX(SCREEN_W/2.0f, tw), by + 35, msg, GLUT_BITMAP_TIMES_ROMAN_24);
    }
    /* Controls list */
    glColor3f(0.90f, 0.95f, 0.90f);
    lineY = by + 65;
    drawText(bx + 30, lineY, "WASD          -  Move", GLUT_BITMAP_HELVETICA_18);       lineY += 22;
    drawText(bx + 30, lineY, "Mouse         -  Look around", GLUT_BITMAP_HELVETICA_18); lineY += 22;
    drawText(bx + 30, lineY, "LMB           -  Shoot", GLUT_BITMAP_HELVETICA_18);      lineY += 22;
    drawText(bx + 30, lineY, "Space         -  Jump", GLUT_BITMAP_HELVETICA_18);       lineY += 22;
    drawText(bx + 30, lineY, "Shift         -  Sprint", GLUT_BITMAP_HELVETICA_18);     lineY += 22;
    drawText(bx + 30, lineY, "R             -  Reload", GLUT_BITMAP_HELVETICA_18);     lineY += 22;
    drawText(bx + 30, lineY, "1 / 2 / 3     -  Switch Weapon", GLUT_BITMAP_HELVETICA_18); lineY += 22;
    drawText(bx + 30, lineY, "Scroll        -  Switch Weapon", GLUT_BITMAP_HELVETICA_18); lineY += 22;
    drawText(bx + 30, lineY, "F5            -  Restart", GLUT_BITMAP_HELVETICA_18);    lineY += 22;
    drawText(bx + 30, lineY, "ESC           -  Quit", GLUT_BITMAP_HELVETICA_18);       lineY += 30;
    /* Press any key */
    {
        const char* hint = "Press any key to start...";
        int tw = textWidth(hint, GLUT_BITMAP_HELVETICA_12);
        glColor3f(0.60f * pulse, 0.80f * pulse, 0.60f * pulse);
        drawText(getCenteredTextX(SCREEN_W/2.0f, tw), lineY, hint, GLUT_BITMAP_HELVETICA_12);
    }
}

static void drawDebugInfo(Player* player) {
    char buf[80];
    glColor4f(0.55f, 0.55f, 0.60f, 0.80f);
    sprintf(buf, "Pos(%.1f, %.1f)  Dir(%.2f, %.2f)", player->x, player->y, player->dirX, player->dirY);
    drawText(12, 18, buf, GLUT_BITMAP_HELVETICA_12);
}

void drawHUD(Player* player) {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0, SCREEN_W, SCREEN_H, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawDamageFlash();
    if (gShowControls) {
        drawControlsPopup();
    } else if (gGameWon) {
        drawVictoryScreen();
    } else if (player->health <= 0) {
        drawGameOver();
    } else {
        drawStatusBarBG();
        drawCrosshair();
        drawHealthBar(player->health);
        drawArmorBar(player->armor);
        drawAmmoPanel();
        drawWeaponSelector();
        drawKillCounter();
        drawItemCollectionPanel();
        drawArenaPanel();
        drawMinimap(player);
        drawDebugInfo(player);
        /* Now Playing song title — kanan bawah kecil */
        {
            const char* np = audioGetNowPlaying();
            if (np && np[0] != '\0') {
                int tw = textWidth(np, GLUT_BITMAP_HELVETICA_12);
                float nx = SCREEN_W - (float)tw * SCREEN_W / (float)(glutGet(GLUT_WINDOW_WIDTH) > 0 ? glutGet(GLUT_WINDOW_WIDTH) : SCREEN_W) - 18.0f;
                float ny = SCREEN_H - 56.0f;
                glColor4f(0.85f, 0.85f, 0.90f, 0.75f); /* slightly brighter too */
                drawText(nx, ny, np, GLUT_BITMAP_HELVETICA_12);
            }
        }
    }
    glDisable(GL_BLEND);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}
