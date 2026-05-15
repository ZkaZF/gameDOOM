#ifndef ITEM_H
#define ITEM_H

#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ITEM_HEALTH  0
#define ITEM_AMMO    1
#define ITEM_ARMOR   2
#define NUM_ITEM_TYPES 3
#define MAX_ITEMS      32
#define ITEM_RADIUS    1.65f   
#define ITEM_BOB_SPEED 2.4f    
#define ITEM_ROT_SPEED 90.0f   
#define ITEM_LIFETIME  25.0f   

typedef struct {
    int   type;
    float x, y;
    float rotAngle;    
    float bobPhase;    
    float lifetime;    
    int   active;
} Item;

static Item  gItems[MAX_ITEMS];
static int   gNumItems  = 0;
static int   gPlayerScore = 0;
static int   gWave        = 1;
static float gWaveTimer   = 0.0f;   

static int   itemGetScore(void)    { return gPlayerScore; }
static int   itemGetWave(void)     { return gWave; }

static void itemSpawn(int type, float x, float y) {
    int i;
    for (i = 0; i < MAX_ITEMS; i++) {
        if (!gItems[i].active) {
            gItems[i].type      = type;
            gItems[i].x         = x;
            gItems[i].y         = y;
            gItems[i].rotAngle  = 0.0f;
            gItems[i].bobPhase  = (float)(rand() % 628) / 100.0f; 
            gItems[i].lifetime  = 0.0f;
            gItems[i].active    = 1;
            if (i >= gNumItems) gNumItems = i + 1;
            return;
        }
    }
}

static void itemTryDrop(float x, float y) {
    int roll = rand() % 100;
    if      (roll < 35) itemSpawn(ITEM_HEALTH, x, y);
    else if (roll < 85) itemSpawn(ITEM_AMMO,   x, y);
    else                itemSpawn(ITEM_ARMOR,   x, y);
}

static void itemInit(void) {
    memset(gItems, 0, sizeof(gItems));
    gNumItems    = 0;
    gPlayerScore = 0;
    gWave        = 1;
    gWaveTimer   = 0.0f;
}

static void itemCheckWave(float dt) {
    int alive = enemyGetAliveCount();
    if (alive > 0) {
        gWaveTimer = 0.0f;
        return;
    }
    gWaveTimer += dt;
    if (gWaveTimer >= 5.0f) {
        int i;
        gWave++;
        gWaveTimer = 0.0f;
        enemyInitLevel();
        gPlayerScore += gWave * 100;
        for (i = 0; i < 2; i++) {
            float px = 18.0f + (float)(rand() % 36);   
            float py = 18.0f + (float)(rand() % 36);
            if (isWalkable((int)px, (int)py))
                itemSpawn(ITEM_HEALTH, px, py);
        }
    }
}

static void itemApplyPickup(Item* it, Player* player) {
    switch (it->type) {
        case ITEM_HEALTH:
            player->health += 30;
            if (player->health > 100) player->health = 100;
            gPlayerScore += 10;
            break;
        case ITEM_AMMO: {
            Weapon* w = &gWeapons[gCurrentWeapon];
            w->ammo += w->maxAmmo / 2;
            if (w->ammo > w->maxAmmo * 2) w->ammo = w->maxAmmo * 2; 
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

static void itemUpdate(Player* player, float dt) {
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
        it->rotAngle  += ITEM_ROT_SPEED * dt;
        if (it->rotAngle >= 360.0f) it->rotAngle -= 360.0f;
        it->bobPhase  += ITEM_BOB_SPEED * dt;
        it->lifetime  += dt;
        
        if (it->lifetime >= ITEM_LIFETIME) {
            it->active = 0;
            continue;
        }
        
        dx   = player->x - it->x;
        dy   = player->y - it->y;
        dist = sqrtf(dx * dx + dy * dy);
        if (dist <= ITEM_RADIUS) {
            itemApplyPickup(it, player);
            it->active = 0;
        }
    }
}


static void renderHealthModel(float bob) {
    glPushMatrix();
    glTranslatef(0.0f, 0.18f + bob, 0.0f);
    
    glColor3f(0.85f, 0.10f, 0.10f);
    glPushMatrix(); glScalef(0.30f, 0.30f, 0.30f); glutSolidCube(1.0f); glPopMatrix();
    
    glColor3f(0.95f, 0.95f, 0.95f);
    glPushMatrix(); glScalef(0.26f, 0.08f, 0.07f); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glScalef(0.08f, 0.26f, 0.07f); glutSolidCube(1.0f); glPopMatrix();
    
    glColor3f(1.0f, 0.45f, 0.45f);
    glPushMatrix(); glScalef(0.32f, 0.32f, 0.32f); glutWireCube(1.0f); glPopMatrix();
    glPopMatrix();
}

static void renderAmmoModel(float bob) {
    glPushMatrix();
    glTranslatef(0.0f, 0.16f + bob, 0.0f);
    
    glColor3f(0.85f, 0.78f, 0.10f);
    glPushMatrix(); glScalef(0.34f, 0.22f, 0.26f); glutSolidCube(1.0f); glPopMatrix();
    
    glColor3f(0.72f, 0.64f, 0.08f);
    glPushMatrix(); glTranslatef(0.0f, 0.12f, 0.0f); glScalef(0.36f, 0.06f, 0.28f); glutSolidCube(1.0f); glPopMatrix();
    
    glColor3f(0.22f, 0.18f, 0.04f);
    glPushMatrix(); glTranslatef(0.0f, 0.0f, 0.14f); glScalef(0.10f, 0.16f, 0.02f); glutSolidCube(1.0f); glPopMatrix();
    glPopMatrix();
}

static void renderArmorModel(float bob) {
    glPushMatrix();
    glTranslatef(0.0f, 0.24f + bob, 0.0f);
    
    glColor3f(0.10f, 0.65f, 0.72f);
    glPushMatrix(); glScalef(0.28f, 0.36f, 0.14f); glutSolidCube(1.0f); glPopMatrix();
    
    glColor3f(0.15f, 0.80f, 0.88f);
    if (gQuad) {
        glPushMatrix();
            glTranslatef(0.0f, 0.20f, 0.0f);
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
            gluCylinder(gQuad, 0.14f, 0.10f, 0.10f, 10, 1);
        glPopMatrix();
    }
    
    glColor3f(0.90f, 0.90f, 0.25f);
    glPushMatrix(); glScalef(0.09f, 0.12f, 0.08f); glutSolidCube(1.0f); glPopMatrix();
    
    glColor3f(0.35f, 0.95f, 1.0f);
    glPushMatrix(); glScalef(0.30f, 0.38f, 0.16f); glutWireCube(1.0f); glPopMatrix();
    glPopMatrix();
}

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
        
        if (it->lifetime > ITEM_LIFETIME - 4.0f) {
            fadeAlpha = (ITEM_LIFETIME - it->lifetime) / 4.0f;
            if (fadeAlpha < 0.0f) fadeAlpha = 0.0f;
        }
        
        bob = sinf(it->bobPhase) * 0.05f * tY; 
        fullH   = (float)SCREEN_H * WALL_HEIGHT_SCALE / tY;
        floorY  = (float)horizY + fullH * 0.5f;
        sScale = 0.40f; 
        spriteH = (int)(fullH * sScale);
        if (spriteH < 2) spriteH = 2;
        spriteW = spriteH;
        sY1 = (int)floorY - (int)(fullH * bob);
        sY0 = sY1 - spriteH;
        sX0 = screenX - spriteW / 2;
        sX1 = screenX + spriteW / 2;
        if (sX1 < 0 || sX0 >= SCREEN_W || sY1 < 0 || sY0 >= SCREEN_H) continue;
        
        behindWall = 1;
        { int step = (spriteW > 16) ? spriteW / 6 : 1;
          for (col = sX0; col <= sX1; col += step)
              if (col >= 0 && col < SCREEN_W && zBuffer[col] >= tY * 0.90f)
                  { behindWall = 0; break; } }
        if (behindWall) continue;
        
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
            glColor4f(0.85f, 0.10f, 0.10f, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(fx0,fy0); glVertex2f(fx1,fy0); glVertex2f(fx1,fy1); glVertex2f(fx0,fy1); glEnd();
            glColor4f(0.95f, 0.95f, 0.95f, fadeAlpha);
            glBegin(GL_QUADS); glVertex2f(midX-w*0.3f, fy0+h*0.4f); glVertex2f(midX+w*0.3f, fy0+h*0.4f); glVertex2f(midX+w*0.3f, fy0+h*0.6f); glVertex2f(midX-w*0.3f, fy0+h*0.6f); glEnd();
            glBegin(GL_QUADS); glVertex2f(midX-w*0.1f, fy0+h*0.2f); glVertex2f(midX+w*0.1f, fy0+h*0.2f); glVertex2f(midX+w*0.1f, fy0+h*0.8f); glVertex2f(midX-w*0.1f, fy0+h*0.8f); glEnd();
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
        }
        if (fadeAlpha < 1.0f) glDisable(GL_BLEND);
    }
}
#endif 
