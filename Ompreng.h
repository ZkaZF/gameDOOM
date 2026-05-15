#ifndef OMPRENG_H
#define OMPRENG_H
#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define ITEM_OMPRENG 10 
#define MAX_OMPRENG 16  
#define OMPRENG_PICKUP_R                                                       \
  1.65f                         
#define OMPRENG_BOB_SPEED 1.6f  
#define OMPRENG_ROT_SPEED 45.0f 
#define OMPRENG_LIFETIME 30.0f  
#define SS_R 0.78f
#define SS_G 0.78f
#define SS_B 0.80f
#define SS_BRIGHT_R 0.92f
#define SS_BRIGHT_G 0.92f
#define SS_BRIGHT_B 0.95f
#define SS_DARK_R 0.55f
#define SS_DARK_G 0.55f
#define SS_DARK_B 0.58f
typedef struct {
  float x, y;     
  float rotAngle; 
  float bobPhase; 
  float lifetime; 
  int active;
} Ompreng;
static Ompreng gOmpreng[MAX_OMPRENG];
static int gNumOmpreng = 0;
static void omprengInit(void) {
  memset(gOmpreng, 0, sizeof(gOmpreng));
  gNumOmpreng = 0;
}
static void omprengSpawn(float x, float y) {
  int i;
  for (i = 0; i < MAX_OMPRENG; i++) {
    if (!gOmpreng[i].active) {
      gOmpreng[i].x = x;
      gOmpreng[i].y = y;
      gOmpreng[i].rotAngle = (float)(rand() % 360);
      gOmpreng[i].bobPhase = (float)(rand() % 628) / 100.0f;
      gOmpreng[i].lifetime = 0.0f;
      gOmpreng[i].active = 1;
      if (i >= gNumOmpreng)
        gNumOmpreng = i + 1;
      return;
    }
  }
}
static void omprengTryDrop(float x, float y, int enemyType) {
  int threshold = (enemyType == 1 ) ? 20 : 5;
  if ((rand() % 100) < threshold) {
    omprengSpawn(x, y);
  }
}
static void omprengApplyPickup(Player *player) {
  player->health += 50;
  if (player->health > 100)
    player->health = 100;
  player->armor += 20;
  if (player->armor > 100)
    player->armor = 100;
}
static void omprengUpdate(Player *player, float dt) {
  int i;
  for (i = 0; i < gNumOmpreng; i++) {
    Ompreng *om = &gOmpreng[i];
    float dx, dy, dist;
    if (!om->active)
      continue;
    om->rotAngle += OMPRENG_ROT_SPEED * dt;
    if (om->rotAngle >= 360.0f)
      om->rotAngle -= 360.0f;
    om->bobPhase += OMPRENG_BOB_SPEED * dt;
    om->lifetime += dt;
    if (om->lifetime >= OMPRENG_LIFETIME) {
      om->active = 0;
      continue;
    }
    dx = player->x - om->x;
    dy = player->y - om->y;
    dist = sqrtf(dx * dx + dy * dy);
    if (dist <= OMPRENG_PICKUP_R) {
      omprengApplyPickup(player);
      om->active = 0;
    }
  }
}
static void omprengCompartmentFloor(float x0, float x1, 
                                    float z0, float z1, 
                                    float y,            
                                    float r, float g, float b) {
  glColor3f(r, g, b);
  glBegin(GL_QUADS);
  glVertex3f(x0, y, z0);
  glVertex3f(x1, y, z0);
  glVertex3f(x1, y, z1);
  glVertex3f(x0, y, z1);
  glEnd();
}
static void omprengDividerX(float x, float z0, float z1, float yBot,
                            float yTop) {
  glBegin(GL_QUADS);
  glVertex3f(x, yBot, z0);
  glVertex3f(x, yBot, z1);
  glVertex3f(x, yTop, z1);
  glVertex3f(x, yTop, z0);
  glEnd();
}
static void omprengDividerZ(float z, float x0, float x1, float yBot,
                            float yTop) {
  glBegin(GL_QUADS);
  glVertex3f(x0, yBot, z);
  glVertex3f(x1, yBot, z);
  glVertex3f(x1, yTop, z);
  glVertex3f(x0, yTop, z);
  glEnd();
}
static void omprengRenderFrame(void) {
  const float W2 = 0.300f;  
  const float D2 = 0.225f;  
  const float BOT = 0.000f; 
  const float TOP = 0.080f; 
  const float MID = 0.055f; 
  const float TH = 0.010f;  
  omprengCompartmentFloor(-W2, W2, -D2, D2, BOT, SS_DARK_R, SS_DARK_G,
                          SS_DARK_B);
  glColor3f(SS_R, SS_G, SS_B);
  omprengDividerZ(-D2, -W2, W2, BOT, TOP);
  omprengDividerZ(D2, -W2, W2, BOT, TOP);
  omprengDividerX(-W2, -D2, D2, BOT, TOP);
  omprengDividerX(W2, -D2, D2, BOT, TOP);
  glColor3f(SS_BRIGHT_R, SS_BRIGHT_G, SS_BRIGHT_B);
  omprengCompartmentFloor(-W2 - TH, W2 + TH, -D2 - TH, -D2, TOP, SS_BRIGHT_R,
                          SS_BRIGHT_G, SS_BRIGHT_B);
  omprengCompartmentFloor(-W2 - TH, W2 + TH, D2, D2 + TH, TOP, SS_BRIGHT_R,
                          SS_BRIGHT_G, SS_BRIGHT_B);
  omprengCompartmentFloor(-W2 - TH, -W2, -D2, D2, TOP, SS_BRIGHT_R, SS_BRIGHT_G,
                          SS_BRIGHT_B);
  omprengCompartmentFloor(W2, W2 + TH, -D2, D2, TOP, SS_BRIGHT_R, SS_BRIGHT_G,
                          SS_BRIGHT_B);
  glColor3f(SS_R, SS_G, SS_B);
  omprengDividerZ(0.0f, -W2, W2, BOT, MID + 0.008f);
  omprengDividerX(-0.100f, -D2, 0.0f, BOT, MID + 0.008f);
  omprengDividerX(0.100f, -D2, 0.0f, BOT, MID + 0.008f);
  omprengDividerX(0.070f, 0.0f, D2, BOT, MID + 0.008f);
  omprengCompartmentFloor(-W2 + 0.005f, -0.105f, -D2 + 0.005f, -0.005f, MID,
                          SS_DARK_R, SS_DARK_G, SS_DARK_B);
  omprengCompartmentFloor(-0.095f, 0.095f, -D2 + 0.005f, -0.005f, MID,
                          SS_DARK_R, SS_DARK_G, SS_DARK_B);
  omprengCompartmentFloor(0.105f, W2 - 0.005f, -D2 + 0.005f, -0.005f, MID,
                          SS_DARK_R, SS_DARK_G, SS_DARK_B);
  omprengCompartmentFloor(-W2 + 0.005f, 0.065f, 0.005f, D2 - 0.005f, MID,
                          SS_DARK_R, SS_DARK_G, SS_DARK_B);
  omprengCompartmentFloor(0.075f, W2 - 0.005f, 0.005f, D2 - 0.005f, MID,
                          SS_DARK_R * 0.9f, SS_DARK_G * 0.9f, SS_DARK_B * 0.9f);
  (void)MID; 
  (void)TH;
}
static void omprengRenderRoundComp(void) {
  if (!gQuad)
    return;
  glColor3f(SS_DARK_R * 0.85f, SS_DARK_G * 0.85f, SS_DARK_B * 0.85f);
  glPushMatrix();
  glTranslatef(0.185f, 0.055f, 0.113f);
  glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); 
  gluDisk(gQuad, 0.0f, 0.095f, 24, 1);
  glPopMatrix();
  glColor3f(SS_R, SS_G, SS_B);
  glPushMatrix();
  glTranslatef(0.185f, 0.055f, 0.113f);
  glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
  gluDisk(gQuad, 0.093f, 0.103f, 24, 1);
  glPopMatrix();
}
static void omprengRenderHighlights(void) {
  glColor3f(SS_BRIGHT_R, SS_BRIGHT_G, SS_BRIGHT_B);
  glLineWidth(1.2f);
  glBegin(GL_LINES);
  glVertex3f(-0.25f, 0.082f, -0.20f);
  glVertex3f(-0.10f, 0.082f, -0.05f);
  glVertex3f(0.00f, 0.082f, -0.18f);
  glVertex3f(0.10f, 0.082f, -0.10f);
  glVertex3f(0.10f, 0.082f, 0.05f);
  glVertex3f(0.25f, 0.082f, 0.18f);
  glEnd();
  glLineWidth(1.0f);
}
static void renderOmprengModel(float bob) {
  glPushMatrix();
  glTranslatef(0.0f, 0.12f + bob, 0.0f);
  omprengRenderFrame();
  omprengRenderRoundComp();
  omprengRenderHighlights();
  glPopMatrix();
}
static void renderOmprengItems(Player *player) {
  int i;
  float det = player->dirX * player->planeY - player->planeX * player->dirY;
  int pitchInt = (int)player->pitch;
  int horizY = SCREEN_H / 2 + pitchInt;
  if (fabsf(det) < 0.00001f)
    det = 0.00001f;
  for (i = 0; i < gNumOmpreng; i++) {
    Ompreng *om = &gOmpreng[i];
    float dx, dy, tX, tY;
    int screenX, behindWall, col;
    float fullH, floorY, bob;
    int spriteH, spriteW, sX0, sX1, sY0, sY1;
    float fx0, fx1, fy0, fy1, midX, w, h;
    float fadeAlpha = 1.0f;
    if (!om->active)
      continue;
    dx = om->x - player->x;
    dy = om->y - player->y;
    tX = (player->dirX * dy - player->dirY * dx) / det;
    tY = (player->planeY * dx - player->planeX * dy) / det;
    if (tY <= 0.1f)
      continue;
    screenX = (int)((float)(SCREEN_W / 2) * (1.0f + tX / tY));
    if (om->lifetime > OMPRENG_LIFETIME - 5.0f) {
      fadeAlpha = (OMPRENG_LIFETIME - om->lifetime) / 5.0f;
      if (fadeAlpha < 0.0f)
        fadeAlpha = 0.0f;
    }
    bob = sinf(om->bobPhase) * 0.06f * tY;
    fullH = (float)SCREEN_H * WALL_HEIGHT_SCALE / tY;
    floorY = (float)horizY + fullH * 0.5f;
    spriteH =
        (int)(fullH * 0.45f); 
    if (spriteH < 2)
      spriteH = 2;
    spriteW = (int)(spriteH * 1.35f); 
    sY1 = (int)floorY - (int)(fullH * bob);
    sY0 = sY1 - spriteH;
    sX0 = screenX - spriteW / 2;
    sX1 = screenX + spriteW / 2;
    if (sX1 < 0 || sX0 >= SCREEN_W || sY1 < 0 || sY0 >= SCREEN_H)
      continue;
    behindWall = 1;
    {
      int step = (spriteW > 16) ? spriteW / 6 : 1;
      for (col = sX0; col <= sX1; col += step)
        if (col >= 0 && col < SCREEN_W && zBuffer[col] >= tY * 0.90f) {
          behindWall = 0;
          break;
        }
    }
    if (behindWall)
      continue;
    fx0 = (float)(sX0 < 0 ? 0 : sX0);
    fx1 = (float)(sX1 >= SCREEN_W ? SCREEN_W - 1 : sX1);
    fy0 = (float)(sY0 < 0 ? 0 : sY0);
    fy1 = (float)(sY1 >= SCREEN_H ? SCREEN_H - 1 : sY1);
    midX = (fx0 + fx1) * 0.5f;
    w = fx1 - fx0;
    h = fy1 - fy0;
    if (w < 1.0f || h < 1.0f)
      continue;
    if (fadeAlpha < 1.0f) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    glColor4f(SS_R, SS_G, SS_B, fadeAlpha);
    glBegin(GL_QUADS);
    glVertex2f(fx0, fy0 + h * 0.12f);
    glVertex2f(fx1, fy0 + h * 0.12f);
    glVertex2f(fx1, fy1);
    glVertex2f(fx0, fy1);
    glEnd();
    glColor4f(SS_BRIGHT_R, SS_BRIGHT_G, SS_BRIGHT_B, fadeAlpha);
    glBegin(GL_QUADS);
    glVertex2f(fx0, fy0);
    glVertex2f(fx1, fy0);
    glVertex2f(fx1, fy0 + h * 0.12f);
    glVertex2f(fx0, fy0 + h * 0.12f);
    glEnd();
    glColor4f(SS_DARK_R, SS_DARK_G, SS_DARK_B, fadeAlpha);
    glBegin(GL_QUADS);
    glVertex2f(fx0, fy0 + h * 0.50f);
    glVertex2f(fx1, fy0 + h * 0.50f);
    glVertex2f(fx1, fy0 + h * 0.54f);
    glVertex2f(fx0, fy0 + h * 0.54f);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(midX - w * 0.33f, fy0 + h * 0.12f);
    glVertex2f(midX - w * 0.30f, fy0 + h * 0.12f);
    glVertex2f(midX - w * 0.30f, fy0 + h * 0.50f);
    glVertex2f(midX - w * 0.33f, fy0 + h * 0.50f);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(midX + w * 0.30f, fy0 + h * 0.12f);
    glVertex2f(midX + w * 0.33f, fy0 + h * 0.12f);
    glVertex2f(midX + w * 0.33f, fy0 + h * 0.50f);
    glVertex2f(midX + w * 0.30f, fy0 + h * 0.50f);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(midX + w * 0.12f, fy0 + h * 0.54f);
    glVertex2f(midX + w * 0.15f, fy0 + h * 0.54f);
    glVertex2f(midX + w * 0.15f, fy1);
    glVertex2f(midX + w * 0.12f, fy1);
    glEnd();
    {
      float cx = midX + w * 0.31f;
      float cy = fy0 + h * 0.77f;
      float cr = h * 0.20f;
      int k;
      glColor4f(SS_DARK_R * 0.88f, SS_DARK_G * 0.88f, SS_DARK_B * 0.88f,
                fadeAlpha);
      glBegin(GL_TRIANGLE_FAN);
      glVertex2f(cx, cy);
      for (k = 0; k <= 12; k++) {
        float ang = (float)k * (2.0f * (float)M_PI / 12.0f);
        glVertex2f(cx + cr * cosf(ang) * 0.85f, cy + cr * sinf(ang));
      }
      glEnd();
    }
    glColor4f(SS_BRIGHT_R, SS_BRIGHT_G, SS_BRIGHT_B, fadeAlpha * 0.6f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f(fx0 + w * 0.10f, fy0 + h * 0.20f);
    glVertex2f(fx0 + w * 0.25f, fy0 + h * 0.35f);
    glEnd();
    glLineWidth(1.0f);
    if (fadeAlpha < 1.0f)
      glDisable(GL_BLEND);
  }
}
#endif 
