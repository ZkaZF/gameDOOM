#ifndef HUD_H
#define HUD_H

/*
 * hud.h — Heads-Up Display (V3)
 *
 * Added in V3:
 *   - Kill counter
 *   - Enemy count remaining
 *   - Damage flash overlay (red screen on hit)
 *   - "YOU DIED" game over screen
 *   - Armor bar
 */

#include <GL/glut.h>
#include <stdio.h>
#include <string.h>

/* ───────────────────── Helpers ───────────────────── */
static void drawText(float x, float y, const char* txt, void* font) {
    const char* c;
    glRasterPos2f(x, y);
    for (c = txt; *c != '\0'; c++)
        glutBitmapCharacter(font, *c);
}

static int textWidth(const char* txt, void* font) {
    int w = 0;
    const char* c;
    for (c = txt; *c != '\0'; c++)
        w += glutBitmapWidth(font, *c);
    return w;
}

static void fillRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
        glVertex2f(x,     y);     glVertex2f(x + w, y);
        glVertex2f(x + w, y + h); glVertex2f(x,     y + h);
    glEnd();
}

static void strokeRect(float x, float y, float w, float h) {
    glBegin(GL_LINE_LOOP);
        glVertex2f(x,     y);     glVertex2f(x + w, y);
        glVertex2f(x + w, y + h); glVertex2f(x,     y + h);
    glEnd();
}

/* ───────────────────── Crosshair ───────────────────── */
static void drawCrosshair(void) {
    float cx = SCREEN_W / 2.0f, cy = SCREEN_H / 2.0f;
    float sz = 10.0f, gap = 4.0f;

    glLineWidth(2.0f);
    /* Shadow */
    glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
    glBegin(GL_LINES);
        glVertex2f(cx-sz-1, cy+1); glVertex2f(cx-gap-1, cy+1);
        glVertex2f(cx+gap+1,cy+1); glVertex2f(cx+sz+1,  cy+1);
        glVertex2f(cx+1, cy-sz-1); glVertex2f(cx+1, cy-gap-1);
        glVertex2f(cx+1, cy+gap+1);glVertex2f(cx+1, cy+sz+1);
    glEnd();
    /* Main */
    glColor3f(0.05f, 0.95f, 0.35f);
    glBegin(GL_LINES);
        glVertex2f(cx-sz, cy); glVertex2f(cx-gap, cy);
        glVertex2f(cx+gap,cy); glVertex2f(cx+sz,  cy);
        glVertex2f(cx, cy-sz); glVertex2f(cx, cy-gap);
        glVertex2f(cx, cy+gap);glVertex2f(cx, cy+sz);
    glEnd();
    glPointSize(2.5f);
    glBegin(GL_POINTS); glVertex2f(cx, cy); glEnd();
    glLineWidth(1.0f); glPointSize(1.0f);
}

/* ───────────────────── Status Bar BG ───────────────────── */
static void drawStatusBarBG(void) {
    float bh = 52.0f;
    glBegin(GL_QUADS);
        glColor4f(0.05f, 0.05f, 0.07f, 0.88f);
        glVertex2f(0,            SCREEN_H - bh);
        glVertex2f(SCREEN_W,     SCREEN_H - bh);
        glColor4f(0.02f, 0.02f, 0.04f, 0.95f);
        glVertex2f(SCREEN_W,     SCREEN_H);
        glVertex2f(0,            SCREEN_H);
    glEnd();
    glColor4f(0.25f, 0.60f, 1.0f, 0.50f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
        glVertex2f(0, SCREEN_H - bh); glVertex2f(SCREEN_W, SCREEN_H - bh);
    glEnd();
    glLineWidth(1.0f);
}

/* ───────────────────── Health Bar ───────────────────── */
static void drawHealthBar(int health) {
    float bx = 20.0f, by = SCREEN_H - 38.0f;
    float bw = 190.0f, bh = 18.0f;
    float r  = (float)health / 100.0f;
    char  txt[32];
    int   seg;

    if (r < 0.0f) r = 0.0f;
    if (r > 1.0f) r = 1.0f;

    /* BG */
    glColor4f(0.08f, 0.08f, 0.08f, 0.90f);
    fillRect(bx - 2, by - 2, bw + 4, bh + 4);

    /* Fill */
    if (r > 0.5f)       glColor3f(0.08f, 0.82f, 0.22f);
    else if (r > 0.25f) glColor3f(0.90f, 0.72f, 0.08f);
    else                glColor3f(0.90f, 0.12f, 0.10f);
    fillRect(bx, by, bw * r, bh);

    /* Dividers */
    glColor4f(0.0f, 0.0f, 0.0f, 0.40f);
    for (seg = 1; seg < 10; seg++) {
        float sx = bx + bw * (float)seg / 10.0f;
        glBegin(GL_LINES); glVertex2f(sx, by); glVertex2f(sx, by + bh); glEnd();
    }

    /* Border */
    glColor4f(0.55f, 0.55f, 0.60f, 0.90f);
    strokeRect(bx, by, bw, bh);

    /* Text */
    glColor3f(0.95f, 0.95f, 0.95f);
    sprintf(txt, "HP %d", health);
    drawText(bx + 6, by + 13, txt, GLUT_BITMAP_HELVETICA_12);
}

/* ─────────────────── Armor Bar ─────────────────── */
static void drawArmorBar(int armor) {
    float bx = 20.0f, by = SCREEN_H - 62.0f;
    float bw = 190.0f, bh = 14.0f;
    float r  = (float)armor / 100.0f;
    char  txt[16];
    if (r < 0.0f) r = 0.0f;
    if (r > 1.0f) r = 1.0f;

    glColor4f(0.08f, 0.08f, 0.08f, 0.85f);
    fillRect(bx - 2, by - 2, bw + 4, bh + 4);

    glColor3f(0.18f, 0.58f, 0.90f);
    fillRect(bx, by, bw * r, bh);

    glColor4f(0.45f, 0.45f, 0.50f, 0.80f);
    strokeRect(bx, by, bw, bh);

    glColor3f(0.75f, 0.85f, 0.95f);
    sprintf(txt, "ARM %d", armor);
    drawText(bx + 6, by + 10, txt, GLUT_BITMAP_HELVETICA_12);
}

/* ─────────────────── Score & Wave ─────────────────── */
static void drawScoreWave(void) {
    char txt[48];
    float px = 20.0f;
    float py = 72.0f;
    float bw = 160.0f, bh = 36.0f;
    int   wave  = itemGetWave();
    int   score = itemGetScore();
    int   alive = enemyGetAliveCount();

    glColor4f(0.05f, 0.05f, 0.05f, 0.72f);
    fillRect(px - 2, py - 2, bw + 4, bh + 4);
    glColor4f(0.20f, 0.50f, 0.85f, 0.55f);
    strokeRect(px - 2, py - 2, bw + 4, bh + 4);

    /* Wave */
    glColor3f(0.55f, 0.80f, 1.0f);
    sprintf(txt, "WAVE %d", wave);
    drawText(px + 6, py + 14, txt, GLUT_BITMAP_HELVETICA_12);

    /* Score */
    glColor3f(0.95f, 0.85f, 0.30f);
    sprintf(txt, "SCORE %d", score);
    drawText(px + 6, py + 28, txt, GLUT_BITMAP_HELVETICA_12);

    /* "NEXT WAVE" flash when enemies are all dead */
    if (alive == 0) {
        double t = (double)glutGet(GLUT_ELAPSED_TIME) / 1000.0;
        float  pulse = 0.55f + 0.45f * (float)(sin(t * 4.0));
        glColor3f(0.30f * pulse, 1.0f * pulse, 0.40f * pulse);
        drawText(px + 4, py + 44, "NEXT WAVE SOON!", GLUT_BITMAP_HELVETICA_12);
    }
}

/* ───────────────────── Ammo Panel ───────────────────── */
static void drawAmmoPanel(void) {
    int   ammo    = weaponGetAmmo();
    int   maxAmmo = weaponGetMaxAmmo();
    float ready   = weaponGetReadyRatio();
    float reload  = weaponGetReloadRatio();
    int   isRel   = weaponIsReloading();
    const char* wname = weaponGetName();
    float panW = 220.0f, panH = 38.0f;
    float px   = SCREEN_W - panW - 16.0f;
    float py   = SCREEN_H - panH - 7.0f;
    float ar   = (maxAmmo > 0) ? (float)ammo / (float)maxAmmo : 0.0f;
    char  txt[32];
    double t = (double)glutGet(GLUT_ELAPSED_TIME) / 1000.0;

    glColor4f(0.08f, 0.08f, 0.08f, 0.90f);
    fillRect(px - 2, py - 2, panW + 4, panH + 4);

    if (isRel) {
        /* Reloading — amber/orange progress bar */
        glColor3f(0.85f, 0.55f, 0.05f);
        fillRect(px, py, (panW - 4) * reload, panH * 0.45f);
    } else {
        /* Normal ammo bar */
        if (ar > 0.5f)       glColor3f(0.15f, 0.55f, 0.90f);
        else if (ar > 0.25f) glColor3f(0.85f, 0.65f, 0.10f);
        else                 glColor3f(0.90f, 0.15f, 0.10f);
        fillRect(px, py, (panW - 4) * ar, panH * 0.45f);

        /* Cooldown ready bar */
        glColor4f(0.05f, 0.90f, 0.40f, 0.65f * ready);
        fillRect(px, py + panH * 0.50f, (panW - 4) * ready, panH * 0.12f);
    }

    glColor4f(0.45f, 0.45f, 0.50f, 0.90f);
    strokeRect(px, py, panW, panH);

    /* Weapon name */
    glColor3f(0.85f, 0.85f, 0.90f);
    drawText(px + 6, py + 14, wname, GLUT_BITMAP_HELVETICA_12);

    if (isRel) {
        /* Pulsing "RELOADING..." */
        float pulse = 0.65f + 0.35f * (float)(sin(t * 6.0));
        glColor3f(1.0f * pulse, 0.70f * pulse, 0.10f * pulse);
        drawText(px + 6, py + 30, "RELOADING...", GLUT_BITMAP_HELVETICA_18);
    } else {
        sprintf(txt, "%d / %d", ammo, maxAmmo);
        glColor3f(0.95f, 0.90f, 0.50f);
        drawText(px + 6, py + 30, txt, GLUT_BITMAP_HELVETICA_18);
    }
}

/* ───────────────────── Weapon Selector ───────────────────── */
static void drawWeaponSelector(void) {
    float iW = 62.0f, iH = 24.0f;
    float sX = SCREEN_W / 2.0f - iW - 4.0f;
    float iY = SCREEN_H - iH - 6.0f;
    const char* names[NUM_WEAPONS] = {"1:PISTOL", "2:SHOTGUN", "3:M416"};
    int i;

    for (i = 0; i < NUM_WEAPONS; i++) {
        float ix  = sX + i * (iW + 4.0f);
        int   act = (i == gCurrentWeapon);

        glColor4f(act ? 0.15f : 0.08f,
                  act ? 0.45f : 0.08f,
                  act ? 0.80f : 0.10f,
                  act ? 0.88f : 0.75f);
        fillRect(ix, iY, iW, iH);

        glColor4f(act ? 0.40f : 0.35f,
                  act ? 0.75f : 0.35f,
                  act ? 1.00f : 0.40f, 1.0f);
        strokeRect(ix, iY, iW, iH);

        glColor3f(act ? 1.0f : 0.60f, act ? 1.0f : 0.60f, act ? 1.0f : 0.65f);
        {
            int tw = textWidth(names[i], GLUT_BITMAP_HELVETICA_12);
            drawText(ix + (iW - tw) / 2.0f, iY + 15, names[i], GLUT_BITMAP_HELVETICA_12);
        }
    }
}

/* ───────────────────── Kill / Alive Counter ───────────────────── */
static void drawKillCounter(void) {
    char txt[64];
    int  kills = enemyGetKillCount();
    int  alive = enemyGetAliveCount();
    float px   = 20.0f;
    float py   = 40.0f;
    float bw   = 160.0f, bh = 36.0f;

    /* Panel BG */
    glColor4f(0.05f, 0.05f, 0.05f, 0.72f);
    fillRect(px - 2, py - 2, bw + 4, bh + 4);

    glColor4f(0.70f, 0.25f, 0.10f, 0.70f);
    strokeRect(px - 2, py - 2, bw + 4, bh + 4);

    /* Kill count */
    glColor3f(0.90f, 0.75f, 0.20f);
    sprintf(txt, "KILLS: %d", kills);
    drawText(px + 6, py + 14, txt, GLUT_BITMAP_HELVETICA_12);

    /* Enemies remaining */
    if (alive > 0) {
        glColor3f(0.90f, 0.30f, 0.20f);
    } else {
        glColor3f(0.20f, 0.90f, 0.30f);
    }
    sprintf(txt, alive > 0 ? "ENEMIES: %d" : "ALL CLEAR!", alive);
    drawText(px + 6, py + 28, txt, GLUT_BITMAP_HELVETICA_12);
}

/* ───────────────────── Minimap ───────────────────── */
static void drawMinimap(Player* player) {
    float mSz = 140.0f;
    float mx  = SCREEN_W - mSz - 14.0f;
    float my  = 14.0f;
    /* Use logical grid dims for cell sizing — world is MAP_SCALE × bigger */
    float cw  = mSz / MAP_LOGICAL_W;
    float ch  = mSz / MAP_LOGICAL_H;
    /* Convert world position to minimap position via MAP_SCALE */
    float invScale = 1.0f / MAP_SCALE;
    int   x, y, i;
    float px, py;

    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    fillRect(mx - 2, my - 2, mSz + 4, mSz + 4);

    /* Walls — iterate logical grid */
    for (y = 0; y < MAP_LOGICAL_H; y++) {
        for (x = 0; x < MAP_LOGICAL_W; x++) {
            int wt = worldMap[y][x];
            if (wt > 0) {
                if (wt >= numWallColors) wt = 1;
                glColor3f(wallColors[wt][0] * 0.75f,
                          wallColors[wt][1] * 0.75f,
                          wallColors[wt][2] * 0.75f);
                fillRect(mx + x * cw, my + y * ch, cw, ch);
            }
        }
    }

    /* Enemies on minimap — convert world pos ÷ MAP_SCALE */
    for (i = 0; i < gNumEnemies; i++) {
        Enemy* e = &gEnemies[i];
        if (!e->active) continue;
        switch (e->type) {
            case ENEMY_IMP:     glColor3f(0.90f, 0.15f, 0.10f); break;
            case ENEMY_DEMON:   glColor3f(0.15f, 0.65f, 0.15f); break;
            case ENEMY_SPECTRE: glColor3f(0.20f, 0.60f, 0.95f); break;
        }
        glPointSize(4.0f);
        glBegin(GL_POINTS);
            glVertex2f(mx + e->x * invScale * cw, my + e->y * invScale * ch);
        glEnd();
    }

    /* Player — convert world pos ÷ MAP_SCALE */
    px = mx + player->x * invScale * cw;
    py = my + player->y * invScale * ch;
    glColor3f(0.10f, 0.95f, 0.30f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
        glVertex2f(px, py);
        glVertex2f(px + player->dirX * 10.0f, py + player->dirY * 10.0f);
    glEnd();
    glLineWidth(1.0f);
    glPointSize(5.5f);
    glBegin(GL_POINTS); glVertex2f(px, py); glEnd();
    glPointSize(1.0f);

    glColor4f(0.30f, 0.55f, 0.80f, 0.70f);
    strokeRect(mx - 2, my - 2, mSz + 4, mSz + 4);
}

/* ───────────────────── Damage Flash ───────────────────── */
static void drawDamageFlash(void) {
    if (gDamageFlash <= 0.0f) return;
    float a = gDamageFlash * 0.45f;
    glColor4f(0.85f, 0.05f, 0.05f, a);
    fillRect(0, 0, SCREEN_W, SCREEN_H);
}

/* ───────────────────── Game Over Screen ───────────────────── */
static void drawGameOver(void) {
    /* Dark overlay */
    glColor4f(0.0f, 0.0f, 0.0f, 0.78f);
    fillRect(0, 0, SCREEN_W, SCREEN_H);

    /* "YOU DIED" */
    {
        const char* msg  = "YOU DIED";
        int tw = textWidth(msg, GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.85f, 0.08f, 0.08f);
        drawText((SCREEN_W - tw) / 2.0f, SCREEN_H / 2.0f - 20, msg, GLUT_BITMAP_TIMES_ROMAN_24);
    }
    {
        const char* sub  = "Press ESC to quit";
        int tw = textWidth(sub, GLUT_BITMAP_HELVETICA_18);
        glColor3f(0.65f, 0.65f, 0.65f);
        drawText((SCREEN_W - tw) / 2.0f, SCREEN_H / 2.0f + 18, sub, GLUT_BITMAP_HELVETICA_18);
    }
}

/* ───────────────────── Debug Info ───────────────────── */
static void drawDebugInfo(Player* player) {
    char buf[80];
    glColor4f(0.55f, 0.55f, 0.60f, 0.80f);
    sprintf(buf, "Pos(%.1f, %.1f)  Dir(%.2f, %.2f)",
            player->x, player->y, player->dirX, player->dirY);
    drawText(12, 18, buf, GLUT_BITMAP_HELVETICA_12);
}

/* ───────────────────── Main HUD ───────────────────── */
static void drawHUD(Player* player) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, SCREEN_W, SCREEN_H, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* Damage flash (behind everything else) */
    drawDamageFlash();

    if (player->health <= 0) {
        drawGameOver();
    } else {
        drawStatusBarBG();
        drawCrosshair();
        drawHealthBar(player->health);
        drawArmorBar(player->armor);
        drawAmmoPanel();
        drawWeaponSelector();
        drawKillCounter();
        drawScoreWave();
        drawMinimap(player);
        drawDebugInfo(player);
    }

    glDisable(GL_BLEND);

    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

#endif /* HUD_H */
