
#include "raycaster.h"
#include "texture.h"
#include "map.h"
#include "player.h"
#include <GL/glut.h>
#include <math.h>

float wallColors[][3] = {
    {0.0f,  0.0f,  0.0f },
    {0.45f, 0.45f, 0.48f},
    {0.65f, 0.25f, 0.18f},
    {0.40f, 0.50f, 0.60f},
    {0.50f, 0.48f, 0.30f},
    {0.55f, 0.38f, 0.22f},
    {0.30f, 0.30f, 0.35f},
};
int   numWallColors = 7;
float zBuffer[SCREEN_W];

static void floorTexColor(float wx, float wy, float *r, float *g, float *b) {
    if (gFloorTex.pixels) {
        sampleBmpTile(&gFloorTex, wx, wy, 5.0f, r, g, b);
    } else {
        int tx = (int)(wx * 0.2f);
        int ty = (int)(wy * 0.2f);
        if ((tx + ty) & 1) { *r = 0.55f; *g = 0.50f; *b = 0.40f; }
        else               { *r = 0.25f; *g = 0.22f; *b = 0.18f; }
    }
}

void renderRaycastView(Player *player) {
    static int floorTexLoaded = 0;
    if (!floorTexLoaded) {
        initFloorTexture();
        initWallTexture();
        floorTexLoaded = 1;
    }
    int x;
    float cameraX, rayDirX, rayDirY;
    int mapX, mapY;
    float sideDistX, sideDistY, deltaDistX, deltaDistY;
    int stepX, stepY, hit, side;
    float perpWallDist;
    int lineHeight, drawStart, drawEnd;
    float r, g, b, shade;
    int wallType;
    int pitchInt = (int)(player->pitch + player->jumpZ * 120.0f);
    int horizY = SCREEN_H / 2 + pitchInt;
    int ceilBot = (horizY < 0) ? 0 : (horizY > SCREEN_H ? SCREEN_H : horizY);
    if (ceilBot > 0) {
        /* Horror fog gradient: soft misty white at top → dark near horizon */
        int numStrips = 8;
        int stripH = ceilBot / numStrips;
        if (stripH < 1) stripH = 1;
        int sy;
        for (sy = 0; sy < ceilBot; sy += stripH) {
            int syEnd = sy + stripH;
            if (syEnd > ceilBot) syEnd = ceilBot;
            /* t=0 at top of screen, t=1 at horizon */
            float t0 = (float)sy / (float)ceilBot;
            float t1 = (float)syEnd / (float)ceilBot;
            /* Fog color: top (0.52, 0.50, 0.48) → horizon (0.08, 0.07, 0.07) */
            float r0 = 0.52f * (1.0f - t0) + 0.08f * t0;
            float g0 = 0.50f * (1.0f - t0) + 0.07f * t0;
            float b0 = 0.48f * (1.0f - t0) + 0.07f * t0;
            float r1 = 0.52f * (1.0f - t1) + 0.08f * t1;
            float g1 = 0.50f * (1.0f - t1) + 0.07f * t1;
            float b1 = 0.48f * (1.0f - t1) + 0.07f * t1;
            glBegin(GL_QUADS);
            glColor3f(r0, g0, b0);
            glVertex2f(0, (float)sy); glVertex2f(SCREEN_W, (float)sy);
            glColor3f(r1, g1, b1);
            glVertex2f(SCREEN_W, (float)syEnd); glVertex2f(0, (float)syEnd);
            glEnd();
        }
    }
    /* Dark base quad for floor — prevents bright gaps between floor pixels */
    if (ceilBot < SCREEN_H) {
        glColor3f(0.04f, 0.03f, 0.03f);
        glBegin(GL_QUADS);
        glVertex2f(0, (float)ceilBot);   glVertex2f(SCREEN_W, (float)ceilBot);
        glVertex2f(SCREEN_W, SCREEN_H);  glVertex2f(0, SCREEN_H);
        glEnd();
    }
    if (ceilBot < SCREEN_H) {
        float dX0 = player->dirX - player->planeX;
        float dY0 = player->dirY - player->planeY;
        float dX1 = player->dirX + player->planeX;
        float dY1 = player->dirY + player->planeY;
        int pRef = SCREEN_H / 2 + pitchInt;
        int y, fogStart = ceilBot;
        {
            int y2;
            for (y2 = ceilBot; y2 < SCREEN_H; y2++) {
                int dy2 = y2 - pRef;
                if (dy2 <= 0) dy2 = 1;
                if ((SCREEN_H * 0.5f * WALL_HEIGHT_SCALE) / (float)dy2 < 50.0f) { fogStart = y2; break; }
            }
        }
        if (fogStart > ceilBot) {
            glColor3f(0.04f, 0.03f, 0.03f);
            glBegin(GL_QUADS);
            glVertex2f(0, (float)ceilBot);   glVertex2f(SCREEN_W, (float)ceilBot);
            glVertex2f(SCREEN_W, (float)fogStart); glVertex2f(0, (float)fogStart);
            glEnd();
        }
        glBegin(GL_QUADS);
        for (y = fogStart; y < SCREEN_H; y += 2) {
            int dy = y - pRef;
            if (dy <= 0) dy = 1;
            float rowDist = (SCREEN_H * 0.5f * WALL_HEIGHT_SCALE) / (float)dy;
            float fog = 1.0f - rowDist / 16.0f;
            if (fog < 0.0f) fog = 0.0f;
            if (fog > 1.0f) fog = 1.0f;
            int skip = (rowDist > 5.0f) ? 4 : 2;
            float sXf = rowDist * (dX1 - dX0) / (float)SCREEN_W;
            float sYf = rowDist * (dY1 - dY0) / (float)SCREEN_W;
            float fX  = player->x + rowDist * dX0;
            float fY  = player->y + rowDist * dY0;
            int fx;
            for (fx = 0; fx < SCREEN_W; fx += skip) {
                float fr, fg2, fb;
                floorTexColor(fX + sXf * fx, fY + sYf * fx, &fr, &fg2, &fb);
                glColor3f(fr * fog, fg2 * fog, fb * fog);
                /* Draw filled rectangle skip×2 to cover all pixels without gaps */
                glVertex2f((float)fx,          (float)y);
                glVertex2f((float)(fx + skip),  (float)y);
                glVertex2f((float)(fx + skip),  (float)(y + 2));
                glVertex2f((float)fx,           (float)(y + 2));
            }
        }
        glEnd();
    }
    glBegin(GL_QUADS);
    for (x = 0; x < SCREEN_W; x++) {
        cameraX = 2.0f * x / (float)SCREEN_W - 1.0f;
        rayDirX = player->dirX + player->planeX * cameraX;
        rayDirY = player->dirY + player->planeY * cameraX;
        mapX = (int)player->x;
        mapY = (int)player->y;
        deltaDistX = (rayDirX == 0) ? 1e30f : fabsf(1.0f / rayDirX);
        deltaDistY = (rayDirY == 0) ? 1e30f : fabsf(1.0f / rayDirY);
        if (rayDirX < 0) { stepX = -1; sideDistX = (player->x - mapX) * deltaDistX; }
        else             { stepX =  1; sideDistX = (mapX + 1.0f - player->x) * deltaDistX; }
        if (rayDirY < 0) { stepY = -1; sideDistY = (player->y - mapY) * deltaDistY; }
        else             { stepY =  1; sideDistY = (mapY + 1.0f - player->y) * deltaDistY; }
        hit = 0; side = 0;
        while (!hit) {
            if (sideDistX < sideDistY) { sideDistX += deltaDistX; mapX += stepX; side = 0; }
            else                       { sideDistY += deltaDistY; mapY += stepY; side = 1; }
            if (getMap(mapX, mapY) > 0) hit = 1;
        }
        perpWallDist = (side == 0)
            ? (mapX - player->x + (1 - stepX) / 2.0f) / rayDirX
            : (mapY - player->y + (1 - stepY) / 2.0f) / rayDirY;
        zBuffer[x] = perpWallDist;
        lineHeight  = (int)((SCREEN_H * WALL_HEIGHT_SCALE) / perpWallDist);
        drawStart   = -lineHeight / 2 + SCREEN_H / 2 + pitchInt;
        if (drawStart < 0) drawStart = 0;
        drawEnd = lineHeight / 2 + SCREEN_H / 2 + pitchInt;
        if (drawEnd >= SCREEN_H) drawEnd = SCREEN_H - 1;
        float wallHitPos;
        if (side == 0) wallHitPos = player->y + perpWallDist * rayDirY;
        else           wallHitPos = player->x + perpWallDist * rayDirX;
        float wallXscaled = wallHitPos / WALL_TEX_TILE_SCALE;
        float u = wallXscaled - (float)((int)wallXscaled);
        if (u < 0.0f) u += 1.0f;
        if ((side == 0 && rayDirX > 0) || (side == 1 && rayDirY < 0)) u = 1.0f - u;
        shade = 1.0f - (perpWallDist / 48.0f);
        if (shade < 0.15f) shade = 0.15f;
        if (shade > 1.0f)  shade = 1.0f;
        float sideDim = (side == 1) ? 0.7f : 1.0f;
        int drawStartFull = -lineHeight / 2 + SCREEN_H / 2 + pitchInt;
        int py;
        for (py = drawStart; py <= drawEnd; py++) {
            if (gWallTex.pixels && gWallTex.width > 0) {
                float v = (float)(py - drawStartFull) / (float)lineHeight;
                if (v < 0.0f) v = 0.0f;
                if (v > 1.0f) v = 1.0f;
                sampleWallBilinear(&gWallTex, u, v, &r, &g, &b);
            } else {
                wallType = getMap(mapX, mapY);
                if (wallType < 0 || wallType >= numWallColors) wallType = 1;
                r = wallColors[wallType][0];
                g = wallColors[wallType][1];
                b = wallColors[wallType][2];
            }
            glColor3f(r * shade * sideDim, g * shade * sideDim, b * shade * sideDim);
            /* 1-pixel wide quad per column — no gaps when window is stretched */
            glVertex2f((float)x,       (float)py);
            glVertex2f((float)(x + 1), (float)py);
            glVertex2f((float)(x + 1), (float)(py + 1));
            glVertex2f((float)x,       (float)(py + 1));
        }
    }
    glEnd();
}
