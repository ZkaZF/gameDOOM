#ifndef RAYCASTER_H
#define RAYCASTER_H

#include <GL/glut.h>
#include <math.h>

/* ───────────────────── Screen Constants ───────────────────── */
#define SCREEN_W 800
#define SCREEN_H 600

/* ───────────────────── Wall Height Scale ───────────────────── */
#define WALL_HEIGHT_SCALE 3.5f

/* ───────────────────── Wall Colors by Type ───────────────────── */
static float wallColors[][3] = {
    {0.0f, 0.0f, 0.0f},
    {0.45f, 0.45f, 0.48f}, /* 1: outer wall — dark gray */
    {0.65f, 0.25f, 0.18f}, /* 2: brick red */
    {0.40f, 0.50f, 0.60f}, /* 3: blue-gray */
    {0.50f, 0.48f, 0.30f}, /* 4: olive */
    {0.55f, 0.38f, 0.22f}, /* 5: wood brown */
    {0.30f, 0.30f, 0.35f}, /* 6: dark metal */
};
static int numWallColors = 7;

/* ───────────────────── Z-Buffer ───────────────────── */
static float zBuffer[SCREEN_W];

/* ───────────────────── Procedural Floor Texture ───────────────── */
/*
 * Checkerboard: cream vs dark-brown. tileSize=3.0 (1 tile = 1 logical cell).
 * Uses int cast instead of floorf (safe since map coords always > 0).
 */
static void floorTexColor(float wx, float wy, float *r, float *g, float *b) {
  /* Integer tile coords — no floorf needed for positive values */
  int tx = (int)(wx / 3.0f);
  int ty = (int)(wy / 3.0f);
  /* Fractional within tile */
  float frx = wx / 3.0f - (float)tx;
  float fry = wy / 3.0f - (float)ty;

  /* Grout lines */
  if (frx < 0.03f || frx > 0.97f || fry < 0.03f || fry > 0.97f) {
    *r = 0.22f;
    *g = 0.21f;
    *b = 0.20f;
    return;
  }
  /* Tiny grain via cheap hash */
  float grain = (float)(((tx * 53 + ty * 97) & 0xFF)) / 255.0f * 0.04f - 0.02f;
  if ((tx + ty) & 1) {
    *r = 0.72f + grain;
    *g = 0.66f + grain;
    *b = 0.52f + grain; /* cream */
  } else {
    *r = 0.28f + grain;
    *g = 0.25f + grain;
    *b = 0.20f + grain; /* dark brown */
  }
}

/* ───────────────────── Render the 3D View ───────────────────── */
static void renderRaycastView(Player *player) {
  int x;
  float cameraX, rayDirX, rayDirY;
  int mapX, mapY;
  float sideDistX, sideDistY, deltaDistX, deltaDistY;
  int stepX, stepY, hit, side;
  float perpWallDist;
  int lineHeight, drawStart, drawEnd;
  float r, g, b, shade;
  int wallType;

  int pitchInt = (int)player->pitch;
  int horizY = SCREEN_H / 2 + pitchInt;
  int ceilBot = (horizY < 0) ? 0 : (horizY > SCREEN_H ? SCREEN_H : horizY);

  /* ── Ceiling: plafon datar putih — jelas terlihat seperti atap ruangan ── */
  if (ceilBot > 0) {
    /* Plafon flat — warna seragam seperti cat tembok putih/abu muda.
     * TIDAK pakai gradient supaya tidak mirip langit. */
    glColor3f(0.82f, 0.82f, 0.80f);   /* putih abu-abu netral */
    glBegin(GL_QUADS);
      glVertex2f(0,        0);
      glVertex2f(SCREEN_W, 0);
      glVertex2f(SCREEN_W, (float)ceilBot);
      glVertex2f(0,        (float)ceilBot);
    glEnd();

    /* Cornice (lis plafon) — garis gelap tebal di bawah plafon.
     * Ini yang bikin otak langsung baca "ini atap", bukan langit. */
    int cornice = (ceilBot > 8) ? 8 : ceilBot;
    glColor3f(0.18f, 0.16f, 0.14f);   /* coklat gelap seperti profil lis kayu */
    glBegin(GL_QUADS);
      glVertex2f(0,        (float)(ceilBot - cornice));
      glVertex2f(SCREEN_W, (float)(ceilBot - cornice));
      glVertex2f(SCREEN_W, (float)ceilBot);
      glVertex2f(0,        (float)ceilBot);
    glEnd();

    /* Highlight tipis di atas cornice (efek 3D profil lis) */
    glColor3f(0.55f, 0.52f, 0.48f);
    glBegin(GL_QUADS);
      glVertex2f(0,        (float)(ceilBot - cornice));
      glVertex2f(SCREEN_W, (float)(ceilBot - cornice));
      glVertex2f(SCREEN_W, (float)(ceilBot - cornice + 2));
      glVertex2f(0,        (float)(ceilBot - cornice + 2));
    glEnd();
  }

  /* ── Floor: batched GL_POINTS, adaptive step ── */
  if (ceilBot < SCREEN_H) {
    float dX0 = player->dirX - player->planeX;
    float dY0 = player->dirY - player->planeY;
    float dX1 = player->dirX + player->planeX;
    float dY1 = player->dirY + player->planeY;
    int pRef = SCREEN_H / 2 + pitchInt;
    /* fogStart = first y-row where rowDist drops below FOG_DIST (30 units).
     * Fewer rows rendered = much faster. Horizon area drawn as dark quad. */
    int y, fogStart = ceilBot;
    {
      int y2;
      for (y2 = ceilBot; y2 < SCREEN_H; y2++) {
        int dy2 = y2 - pRef;
        if (dy2 <= 0)
          dy2 = 1;
        if ((SCREEN_H * 0.5f * WALL_HEIGHT_SCALE) / (float)dy2 < 50.0f) {
          fogStart = y2;
          break;
        }
      }
    }

    /* Dark band: horizon → fogStart */
    if (fogStart > ceilBot) {
      glColor3f(0.06f, 0.05f, 0.04f);
      glBegin(GL_QUADS);
      glVertex2f(0, (float)ceilBot);
      glVertex2f(SCREEN_W, (float)ceilBot);
      glVertex2f(SCREEN_W, (float)fogStart);
      glVertex2f(0, (float)fogStart);
      glEnd();
    }

    /* Textured floor — y-step 2 (each row covers 2px), adaptive x-skip */
    glBegin(GL_POINTS);
    for (y = fogStart; y < SCREEN_H; y += 2) {
      int dy = y - pRef;
      if (dy <= 0)
        dy = 1;
      float rowDist = (SCREEN_H * 0.5f * WALL_HEIGHT_SCALE) / (float)dy;
      float fog = 1.0f - rowDist / 50.0f;
      if (fog < 0.0f)
        fog = 0.0f;
      if (fog > 1.0f)
        fog = 1.0f;

      float sXf = rowDist * (dX1 - dX0) / (float)SCREEN_W;
      float sYf = rowDist * (dY1 - dY0) / (float)SCREEN_W;
      float fX = player->x + rowDist * dX0;
      float fY = player->y + rowDist * dY0;

      /* Aggressive skip: 2 near, 4 mid, 8 far */
      int skip = (rowDist > 15.0f) ? 8 : (rowDist > 7.0f) ? 4 : 2;
      int fx;
      for (fx = 0; fx < SCREEN_W; fx += skip) {
        float fr, fg2, fb;
        floorTexColor(fX + sXf * fx, fY + sYf * fx, &fr, &fg2, &fb);
        glColor3f(fr * fog, fg2 * fog, fb * fog);
        /* Draw 2 rows × skip cols */
        glVertex2f((float)fx, (float)y);
        glVertex2f((float)fx, (float)(y + 1));
        if (skip >= 2) {
          glVertex2f((float)(fx + 1), (float)y);
          glVertex2f((float)(fx + 1), (float)(y + 1));
        }
        if (skip >= 4) {
          glVertex2f((float)(fx + 2), (float)y);
          glVertex2f((float)(fx + 2), (float)(y + 1));
          glVertex2f((float)(fx + 3), (float)y);
          glVertex2f((float)(fx + 3), (float)(y + 1));
        }
        if (skip >= 8) {
          int k;
          for (k = 4; k < 8 && fx + k < SCREEN_W; k++) {
            glVertex2f((float)(fx + k), (float)y);
            glVertex2f((float)(fx + k), (float)(y + 1));
          }
        }
      }
    }
    glEnd();
  }

  /* ── Walls: all 800 strips in ONE glBegin(GL_LINES) ── */
  glBegin(GL_LINES);
  for (x = 0; x < SCREEN_W; x++) {
    cameraX = 2.0f * x / (float)SCREEN_W - 1.0f;
    rayDirX = player->dirX + player->planeX * cameraX;
    rayDirY = player->dirY + player->planeY * cameraX;

    mapX = (int)player->x;
    mapY = (int)player->y;

    deltaDistX = (rayDirX == 0) ? 1e30f : fabsf(1.0f / rayDirX);
    deltaDistY = (rayDirY == 0) ? 1e30f : fabsf(1.0f / rayDirY);

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

    hit = 0;
    side = 0;
    while (!hit) {
      if (sideDistX < sideDistY) {
        sideDistX += deltaDistX;
        mapX += stepX;
        side = 0;
      } else {
        sideDistY += deltaDistY;
        mapY += stepY;
        side = 1;
      }
      if (getMap(mapX, mapY) > 0)
        hit = 1;
    }

    perpWallDist = (side == 0)
                       ? (mapX - player->x + (1 - stepX) / 2.0f) / rayDirX
                       : (mapY - player->y + (1 - stepY) / 2.0f) / rayDirY;

    zBuffer[x] = perpWallDist;

    lineHeight = (int)((SCREEN_H * WALL_HEIGHT_SCALE) / perpWallDist);
    drawStart = -lineHeight / 2 + SCREEN_H / 2 + pitchInt;
    if (drawStart < 0)
      drawStart = 0;
    drawEnd = lineHeight / 2 + SCREEN_H / 2 + pitchInt;
    if (drawEnd >= SCREEN_H)
      drawEnd = SCREEN_H - 1;

    wallType = getMap(mapX, mapY);
    if (wallType < 0 || wallType >= numWallColors)
      wallType = 1;
    r = wallColors[wallType][0];
    g = wallColors[wallType][1];
    b = wallColors[wallType][2];
    if (side == 1) {
      r *= 0.7f;
      g *= 0.7f;
      b *= 0.7f;
    }
    shade = 1.0f - (perpWallDist / 48.0f);
    if (shade < 0.15f)
      shade = 0.15f;
    if (shade > 1.0f)
      shade = 1.0f;

    glColor3f(r * shade, g * shade, b * shade);
    glVertex2f((float)x, (float)drawStart);
    glVertex2f((float)x, (float)drawEnd);
  }
  glEnd();
}

#endif /* RAYCASTER_H */
