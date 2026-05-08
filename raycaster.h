#ifndef RAYCASTER_H
#define RAYCASTER_H

#include <math.h>
#include <GL/glut.h>

/* ───────────────────── Screen Constants ───────────────────── */
#define SCREEN_W 800
#define SCREEN_H 600

/* ───────────────────── Wall Colors by Type ───────────────────── */
/* Each wall type has RGB color. Index = wall type (1-6) */
static float wallColors[][3] = {
    {0.0f, 0.0f, 0.0f},       /* 0: unused */
    {0.45f, 0.45f, 0.48f},    /* 1: outer wall — dark gray concrete */
    {0.65f, 0.25f, 0.18f},    /* 2: building A — brick red */
    {0.40f, 0.50f, 0.60f},    /* 3: building B — blue-gray */
    {0.50f, 0.48f, 0.30f},    /* 4: barrier — olive/sandbag */
    {0.55f, 0.38f, 0.22f},    /* 5: interior — wood brown */
    {0.30f, 0.30f, 0.35f},    /* 6: accent — dark metal */
};
static int numWallColors = 7;

/* ───────────────────── Z-Buffer for sprite rendering ───────────── */
static float zBuffer[SCREEN_W];

/* ───────────────────── Render the 3D View ───────────────────── */
static void renderRaycastView(Player* player) {
    int x;
    float cameraX, rayDirX, rayDirY;
    int mapX, mapY;
    float sideDistX, sideDistY;
    float deltaDistX, deltaDistY;
    int stepX, stepY;
    int hit, side;
    float perpWallDist;
    int lineHeight, drawStart, drawEnd;
    float r, g, b;
    int wallType;
    float shade;

    /* ─── Draw ceiling (top half, shifted by pitch) ─── */
    {
        int horizY = SCREEN_H / 2 + (int)player->pitch;
        int ceilBot = (horizY < 0) ? 0 : (horizY > SCREEN_H ? SCREEN_H : horizY);
        if (ceilBot > 0) {
            glBegin(GL_QUADS);
                glColor3f(0.05f, 0.05f, 0.12f);
                glVertex2f(0, 0);
                glVertex2f(SCREEN_W, 0);
                glColor3f(0.15f, 0.12f, 0.20f);
                glVertex2f(SCREEN_W, (float)ceilBot);
                glVertex2f(0,        (float)ceilBot);
            glEnd();
        }
    }

    /* ─── Draw floor (bottom half, shifted by pitch) ─── */
    {
        int horizY = SCREEN_H / 2 + (int)player->pitch;
        int floorTop = (horizY < 0) ? 0 : (horizY > SCREEN_H ? SCREEN_H : horizY);
        if (floorTop < SCREEN_H) {
            glBegin(GL_QUADS);
                glColor3f(0.18f, 0.18f, 0.16f);
                glVertex2f(0,        (float)floorTop);
                glVertex2f(SCREEN_W, (float)floorTop);
                glColor3f(0.10f, 0.10f, 0.08f);
                glVertex2f(SCREEN_W, SCREEN_H);
                glVertex2f(0,        SCREEN_H);
            glEnd();
        }
    }

    /* ─── Cast rays for each column ─── */
    for (x = 0; x < SCREEN_W; x++) {
        /* Calculate ray position and direction */
        cameraX = 2.0f * x / (float)SCREEN_W - 1.0f; /* -1 to +1 */
        rayDirX = player->dirX + player->planeX * cameraX;
        rayDirY = player->dirY + player->planeY * cameraX;

        /* Current map cell */
        mapX = (int)player->x;
        mapY = (int)player->y;

        /* Length of ray from one x/y-side to next x/y-side (avoid div by zero) */
        deltaDistX = (rayDirX == 0) ? 1e30f : fabsf(1.0f / rayDirX);
        deltaDistY = (rayDirY == 0) ? 1e30f : fabsf(1.0f / rayDirY);

        /* Calculate step and initial sideDist */
        if (rayDirX < 0) {
            stepX = -1;
            sideDistX = (player->x - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0f - player->x) * deltaDistX;
        }
        if (rayDirY < 0) {
            stepY = -1;
            sideDistY = (player->y - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0f - player->y) * deltaDistY;
        }

        /* ─── DDA Algorithm ─── */
        hit = 0;
        side = 0;
        while (!hit) {
            /* Jump to next map square */
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0; /* hit X-side (vertical wall) */
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1; /* hit Y-side (horizontal wall) */
            }
            /* Check if ray hit a wall */
            if (getMap(mapX, mapY) > 0) {
                hit = 1;
            }
        }

        /* Calculate perpendicular wall distance (avoid fisheye) */
        if (side == 0) {
            perpWallDist = (mapX - player->x + (1 - stepX) / 2.0f) / rayDirX;
        } else {
            perpWallDist = (mapY - player->y + (1 - stepY) / 2.0f) / rayDirY;
        }

        /* Store in z-buffer for sprite rendering later */
        zBuffer[x] = perpWallDist;

        /* Calculate wall strip height + apply pitch shift */
        lineHeight = (int)(SCREEN_H / perpWallDist);
        {
            int pitchOff = (int)player->pitch;
            drawStart = -lineHeight / 2 + SCREEN_H / 2 + pitchOff;
            if (drawStart < 0) drawStart = 0;
            drawEnd   =  lineHeight / 2 + SCREEN_H / 2 + pitchOff;
            if (drawEnd >= SCREEN_H) drawEnd = SCREEN_H - 1;
        }

        /* Get wall color based on type */
        wallType = getMap(mapX, mapY);
        if (wallType < 0 || wallType >= numWallColors) wallType = 1;

        r = wallColors[wallType][0];
        g = wallColors[wallType][1];
        b = wallColors[wallType][2];

        /* Side shading — Y-side walls are darker for depth perception */
        if (side == 1) {
            r *= 0.7f;
            g *= 0.7f;
            b *= 0.7f;
        }

        /* Distance fog — darken walls further away */
        shade = 1.0f - (perpWallDist / 16.0f);
        if (shade < 0.15f) shade = 0.15f;
        if (shade > 1.0f) shade = 1.0f;
        r *= shade;
        g *= shade;
        b *= shade;

        /* ─── Draw the wall strip ─── */
        glColor3f(r, g, b);
        glBegin(GL_LINES);
            glVertex2f((float)x, (float)drawStart);
            glVertex2f((float)x, (float)drawEnd);
        glEnd();
    }
}

#endif /* RAYCASTER_H */
