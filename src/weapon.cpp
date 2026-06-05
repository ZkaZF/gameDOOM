
#include "weapon.h"
#include "player.h"
#include "map.h"
#include "raycaster.h"
#include "audio.h"
#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

Weapon      gWeapons[NUM_WEAPONS];
int         gCurrentWeapon = WEAPON_NONE;
Projectile  gProjectiles[MAX_PROJECTILES];
GLUquadric* gQuad = NULL;
float       gMuzzleFlash = 0.0f;
float       gOutOfAmmoTimer = 0.0f;

/* Inisialisasi statistik, amunisi, waktu reload, dan model 3D untuk semua senjata yang tersedia */
void weaponInit(void) {
    memset(gProjectiles, 0, sizeof(gProjectiles));
    gWeapons[WEAPON_PISTOL].type           = WEAPON_PISTOL;
    strcpy(gWeapons[WEAPON_PISTOL].name, "PISTOL");
    gWeapons[WEAPON_PISTOL].ammo           = 12;
    gWeapons[WEAPON_PISTOL].maxAmmo        = 12;
    gWeapons[WEAPON_PISTOL].cooldown       = 0.35f;
    gWeapons[WEAPON_PISTOL].timer          = 9999.0f;
    gWeapons[WEAPON_PISTOL].recoilY        = 0.0f;
    gWeapons[WEAPON_PISTOL].spreadAngle    = 0.01f;
    gWeapons[WEAPON_PISTOL].pelletsPerShot = 1;
    gWeapons[WEAPON_PISTOL].damage         = 20;
    gWeapons[WEAPON_PISTOL].projR          = 1.0f;
    gWeapons[WEAPON_PISTOL].projG          = 0.85f;
    gWeapons[WEAPON_PISTOL].projB          = 0.1f;
    gWeapons[WEAPON_PISTOL].isReloading    = 0;
    gWeapons[WEAPON_PISTOL].reloadTimer    = 0.0f;
    gWeapons[WEAPON_PISTOL].reloadDuration = 1.2f;
    gWeapons[WEAPON_PISTOL].reloadY        = 0.0f;
    gWeapons[WEAPON_PISTOL].unlocked       = 0;
    gWeapons[WEAPON_PISTOL].reserveAmmo    = 0;
    gWeapons[WEAPON_PISTOL].maxReserveAmmo = 48;  /* 4 mags */
    gWeapons[WEAPON_SHOTGUN].type           = WEAPON_SHOTGUN;
    strcpy(gWeapons[WEAPON_SHOTGUN].name, "SHOTGUN");
    gWeapons[WEAPON_SHOTGUN].ammo           = 6;
    gWeapons[WEAPON_SHOTGUN].maxAmmo        = 6;
    gWeapons[WEAPON_SHOTGUN].cooldown       = 0.90f;
    gWeapons[WEAPON_SHOTGUN].timer          = 9999.0f;
    gWeapons[WEAPON_SHOTGUN].recoilY        = 0.0f;
    gWeapons[WEAPON_SHOTGUN].spreadAngle    = 0.13f;
    gWeapons[WEAPON_SHOTGUN].pelletsPerShot = 7;
    gWeapons[WEAPON_SHOTGUN].damage         = 15;
    gWeapons[WEAPON_SHOTGUN].projR          = 1.0f;
    gWeapons[WEAPON_SHOTGUN].projG          = 0.50f;
    gWeapons[WEAPON_SHOTGUN].projB          = 0.10f;
    gWeapons[WEAPON_SHOTGUN].isReloading    = 0;
    gWeapons[WEAPON_SHOTGUN].reloadTimer    = 0.0f;
    gWeapons[WEAPON_SHOTGUN].reloadDuration = 2.2f;
    gWeapons[WEAPON_SHOTGUN].reloadY        = 0.0f;
    gWeapons[WEAPON_SHOTGUN].unlocked       = 0;
    gWeapons[WEAPON_SHOTGUN].reserveAmmo    = 0;
    gWeapons[WEAPON_SHOTGUN].maxReserveAmmo = 18;  /* 3 mags */
    gWeapons[WEAPON_M416].type           = WEAPON_M416;
    strcpy(gWeapons[WEAPON_M416].name, "M416");
    gWeapons[WEAPON_M416].ammo           = 30;
    gWeapons[WEAPON_M416].maxAmmo        = 30;
    gWeapons[WEAPON_M416].cooldown       = 0.092f;
    gWeapons[WEAPON_M416].timer          = 9999.0f;
    gWeapons[WEAPON_M416].recoilY        = 0.0f;
    gWeapons[WEAPON_M416].spreadAngle    = 0.025f;
    gWeapons[WEAPON_M416].pelletsPerShot = 1;
    gWeapons[WEAPON_M416].damage         = 15;
    gWeapons[WEAPON_M416].projR          = 0.55f;
    gWeapons[WEAPON_M416].projG          = 0.95f;
    gWeapons[WEAPON_M416].projB          = 0.30f;
    gWeapons[WEAPON_M416].isReloading    = 0;
    gWeapons[WEAPON_M416].reloadTimer    = 0.0f;
    gWeapons[WEAPON_M416].reloadDuration = 2.4f;
    gWeapons[WEAPON_M416].reloadY        = 0.0f;
    gWeapons[WEAPON_M416].unlocked       = 0;
    gWeapons[WEAPON_M416].reserveAmmo    = 0;
    gWeapons[WEAPON_M416].maxReserveAmmo = 120;  /* 4 mags */
    gQuad = gluNewQuadric();
    gluQuadricDrawStyle(gQuad, GLU_FILL);
    gluQuadricNormals(gQuad, GLU_SMOOTH);
}

/* Memunculkan proyektil/peluru ke dunia game pada koordinat dan arah tertentu saat menembak */
void spawnProjectile(float x, float y, float dx, float dy,
                     int dmg, float r, float g, float b) {
    int i;
    for (i = 0; i < MAX_PROJECTILES; i++) {
        if (!gProjectiles[i].active) {
            gProjectiles[i].x      = x;
            gProjectiles[i].y      = y;
            gProjectiles[i].dirX   = dx;
            gProjectiles[i].dirY   = dy;
            gProjectiles[i].dist   = 0.0f;
            gProjectiles[i].active = 1;
            gProjectiles[i].damage = dmg;
            gProjectiles[i].r      = r;
            gProjectiles[i].g      = g;
            gProjectiles[i].b      = b;
            return;
        }
    }
}

/* Menembakkan senjata saat ini, mengurangi ammo, memunculkan proyektil (atau pelet shotgun), dan efek visual/SFX */
void weaponShoot(Player* player) {
    Weapon* w;
    int i;
    if (gCurrentWeapon == WEAPON_NONE) return;
    w = &gWeapons[gCurrentWeapon];
    if (w->isReloading)         return;
    if (w->timer < w->cooldown) return;
    if (w->ammo <= 0) {
        if (!w->isReloading) {
            if (w->reserveAmmo > 0) {
                w->isReloading = 1; w->reloadTimer = 0.0f;
            } else {
                gOutOfAmmoTimer = 1.5f;
            }
        }
        return;
    }
    /* Fire! */
    w->ammo--;
    w->timer   = 0.0f;
    w->recoilY = (gCurrentWeapon == WEAPON_SHOTGUN) ? 0.20f :
                 (gCurrentWeapon == WEAPON_M416)    ? 0.04f : 0.10f;
    gMuzzleFlash = 0.10f;
    sfxShoot(gCurrentWeapon);
    for (i = 0; i < w->pelletsPerShot; i++) {
        float pDirX, pDirY;
        float spread = w->spreadAngle;
        /* ADS reduces spread significantly for M416 */
        if (gCurrentWeapon == WEAPON_M416 && player->isADS) spread *= 0.25f;
        if (w->pelletsPerShot == 1) {
            pDirX = player->dirX; pDirY = player->dirY;
        } else {
            float t     = (w->pelletsPerShot > 1) ? (float)i / (float)(w->pelletsPerShot - 1) : 0.5f;
            float angle = -spread + t * (2.0f * spread);
            float cosA  = cosf(angle); float sinA = sinf(angle);
            pDirX = player->dirX * cosA - player->dirY * sinA;
            pDirY = player->dirX * sinA + player->dirY * cosA;
        }
        spawnProjectile(player->x + pDirX * 0.35f, player->y + pDirY * 0.35f,
                        pDirX, pDirY, w->damage, w->projR, w->projG, w->projB);
    }
}

/* Memulai proses reload senjata saat ini jika ada sisa amunisi cadangan (reserve) */
void weaponReload(void) {
    Weapon* w;
    int needed, take;
    if (gCurrentWeapon == WEAPON_NONE) return;
    w = &gWeapons[gCurrentWeapon];
    if (w->isReloading) return;
    if (w->ammo >= w->maxAmmo) return;
    if (w->reserveAmmo <= 0) {
        if (w->ammo <= 0) gOutOfAmmoTimer = 1.5f;
        return;  /* no reserve ammo left */
    }
    w->isReloading = 1;
    w->reloadTimer = 0.0f;
    sfxReload();
}

/* Mengganti senjata yang sedang digunakan ke indeks senjata yang baru (jika sudah terbuka) */
void weaponSwitch(int index) {
    if (index < 0 || index >= NUM_WEAPONS) return;
    if (!gWeapons[index].unlocked) return;
    gCurrentWeapon = index;
    gWeapons[gCurrentWeapon].recoilY = 0.0f;
}

/* Mengupdate timer cooldown tembakan, animasi reload, posisi proyektil, dan collision proyektil tiap frame */
void weaponUpdate(float dt) {
    int i;
    Weapon* w;
    if (gOutOfAmmoTimer > 0.0f) {
        gOutOfAmmoTimer -= dt;
        if (gOutOfAmmoTimer < 0.0f) gOutOfAmmoTimer = 0.0f;
    }
    if (gCurrentWeapon == WEAPON_NONE) {
        /* Still update projectiles even with no weapon */
        for (i = 0; i < MAX_PROJECTILES; i++) {
            Projectile* p = &gProjectiles[i];
            if (!p->active) continue;
            p->x    += p->dirX * PROJ_SPEED;
            p->y    += p->dirY * PROJ_SPEED;
            p->dist += PROJ_SPEED;
            if (p->dist >= PROJ_MAX_DIST)          { p->active = 0; continue; }
            if (getMap((int)p->x, (int)p->y) > 0) { p->active = 0; continue; }
        }
        return;
    }
    w = &gWeapons[gCurrentWeapon];
    w->timer += dt;
    if (w->recoilY > 0.0f) {
        w->recoilY -= dt * 1.2f;
        if (w->recoilY < 0.0f) w->recoilY = 0.0f;
    }
    if (w->isReloading) {
        w->reloadTimer += dt;
        {
            float t  = w->reloadTimer / w->reloadDuration;
            float hl = 0.5f;
            if (t < hl) w->reloadY = (t / hl) * 0.45f;
            else        w->reloadY = (1.0f - (t - hl) / hl) * 0.45f;
            if (w->reloadY < 0.0f) w->reloadY = 0.0f;
        }
        if (w->reloadTimer >= w->reloadDuration) {
            int needed = w->maxAmmo - w->ammo;
            int take = (needed < w->reserveAmmo) ? needed : w->reserveAmmo;
            w->ammo       += take;
            w->reserveAmmo -= take;
            w->isReloading = 0;
            w->reloadY     = 0.0f;
            w->reloadTimer = 0.0f;
        }
    } else {
        w->reloadY = 0.0f;
    }
    if (gMuzzleFlash > 0.0f) {
        gMuzzleFlash -= dt * 2.5f;
        if (gMuzzleFlash < 0.0f) gMuzzleFlash = 0.0f;
    }
    for (i = 0; i < MAX_PROJECTILES; i++) {
        Projectile* p = &gProjectiles[i];
        if (!p->active) continue;
        p->x    += p->dirX * PROJ_SPEED;
        p->y    += p->dirY * PROJ_SPEED;
        p->dist += PROJ_SPEED;
        if (p->dist >= PROJ_MAX_DIST)          { p->active = 0; continue; }
        if (getMap((int)p->x, (int)p->y) > 0) { p->active = 0; continue; }
    }
}

static void renderPistolModel(void) {
    glColor3f(0.22f, 0.22f, 0.26f);
    glPushMatrix(); glScalef(0.11f, 0.08f, 0.30f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.16f, 0.16f, 0.18f);
    glPushMatrix(); glTranslatef(0.0f, 0.016f, -0.22f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(gQuad, 0.024f, 0.021f, 0.20f, 10, 1); glPopMatrix();
    glColor3f(0.28f, 0.17f, 0.08f);
    glPushMatrix(); glTranslatef(0.0f, -0.092f, 0.04f); glRotatef(14.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.09f, 0.16f, 0.10f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.20f, 0.20f, 0.23f);
    glPushMatrix(); glTranslatef(0.0f, -0.052f, 0.02f); glScalef(0.075f, 0.035f, 0.12f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.9f, 0.9f, 0.9f);
    glPushMatrix(); glTranslatef(0.0f, 0.050f, -0.12f); glScalef(0.014f, 0.020f, 0.018f); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.022f, 0.048f, 0.10f); glScalef(0.012f, 0.018f, 0.014f); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.022f, 0.048f, 0.10f); glScalef(0.012f, 0.018f, 0.014f); glutSolidCube(1.0f); glPopMatrix();
}

static void renderShotgunModel(void) {
    glColor3f(0.26f, 0.20f, 0.13f);
    glPushMatrix(); glTranslatef(0.0f, 0.0f, 0.06f); glScalef(0.17f, 0.13f, 0.48f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.18f, 0.18f, 0.20f);
    glPushMatrix(); glTranslatef(-0.038f, 0.022f, -0.30f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(gQuad, 0.028f, 0.025f, 0.45f, 10, 1); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.038f, 0.022f, -0.30f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(gQuad, 0.028f, 0.025f, 0.45f, 10, 1); glPopMatrix();
    glColor3f(0.33f, 0.26f, 0.16f);
    glPushMatrix(); glTranslatef(0.0f, 0.002f, -0.14f); glScalef(0.16f, 0.11f, 0.16f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.30f, 0.22f, 0.12f);
    glPushMatrix(); glTranslatef(0.0f, -0.028f, 0.32f); glRotatef(-7.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.14f, 0.15f, 0.24f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.18f, 0.18f, 0.20f);
    glPushMatrix(); glTranslatef(0.0f, -0.075f, 0.06f); glScalef(0.07f, 0.04f, 0.16f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.90f, 0.75f, 0.10f);
    glPushMatrix(); glTranslatef(0.0f, 0.068f, -0.27f); glutSolidSphere(0.013f, 8, 6); glPopMatrix();
    glColor3f(0.15f, 0.15f, 0.17f);
    glPushMatrix(); glTranslatef(0.09f, 0.04f, 0.04f); glScalef(0.02f, 0.06f, 0.10f); glutSolidCube(1.0f); glPopMatrix();
}

static void renderM416Model(void) {
    glColor3f(0.20f, 0.22f, 0.20f);
    glPushMatrix(); glScalef(0.10f, 0.09f, 0.48f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.15f, 0.16f, 0.15f);
    glPushMatrix(); glTranslatef(0.0f, 0.012f, -0.38f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(gQuad, 0.018f, 0.016f, 0.32f, 10, 1); glPopMatrix();
    glColor3f(0.18f, 0.19f, 0.17f);
    glPushMatrix(); glTranslatef(0.0f, 0.038f, -0.30f); glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(gQuad, 0.009f, 0.009f, 0.26f, 8, 1); glPopMatrix();
    glColor3f(0.25f, 0.27f, 0.24f);
    glPushMatrix(); glTranslatef(0.0f, 0.006f, -0.22f); glScalef(0.13f, 0.10f, 0.25f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.14f, 0.15f, 0.13f);
    glPushMatrix(); glTranslatef(0.0f, -0.13f, 0.04f); glRotatef(7.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.07f, 0.20f, 0.10f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.18f, 0.14f, 0.10f);
    glPushMatrix(); glTranslatef(0.0f, -0.10f, 0.15f); glRotatef(18.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.07f, 0.15f, 0.09f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.20f, 0.22f, 0.20f);
    glPushMatrix(); glTranslatef(0.0f, 0.0f, 0.30f); glScalef(0.08f, 0.06f, 0.20f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.12f, 0.12f, 0.12f);
    glPushMatrix(); glTranslatef(0.0f, -0.018f, 0.41f); glScalef(0.09f, 0.09f, 0.04f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.22f, 0.24f, 0.22f);
    glPushMatrix(); glTranslatef(0.0f, 0.058f, 0.05f); glScalef(0.06f, 0.04f, 0.30f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.90f, 0.90f, 0.88f);
    glPushMatrix(); glTranslatef(0.0f, 0.076f, -0.17f); glScalef(0.010f, 0.022f, 0.012f); glutSolidCube(1.0f); glPopMatrix();
    glColor3f(0.15f, 0.16f, 0.15f);
    glPushMatrix(); glTranslatef(0.07f, 0.008f, 0.11f); glScalef(0.025f, 0.025f, 0.06f); glutSolidCube(1.0f); glPopMatrix();
}

/* Render 3D model senjata di tangan pemain, dengan efek bobbing (naik turun), recoil, dan muzzle flash */
void renderWeapon3D(Player* player) {
    Weapon* w;
    float aspect;
    double t;
    int isMoving;
    if (gCurrentWeapon == WEAPON_NONE) return;  /* bare hands */
    w    = &gWeapons[gCurrentWeapon];
    aspect = (float)SCREEN_W / (float)SCREEN_H;
    t     = (double)glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    isMoving = player->moveForward || player->moveBackward ||
                   player->strafeLeft  || player->strafeRight;
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    {
        GLfloat lPos[] = { 1.0f, 3.0f, 1.5f, 0.0f };
        GLfloat lAmb[] = { 0.35f, 0.32f, 0.28f, 1.0f };
        GLfloat lDif[] = { 0.85f, 0.80f, 0.70f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, lPos);
        glLightfv(GL_LIGHT0, GL_AMBIENT,  lAmb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  lDif);
    }
    if (gMuzzleFlash > 0.0f) {
        GLfloat fPos[] = { 0.0f, 0.0f, -1.0f, 1.0f };
        float   fi     = gMuzzleFlash * 10.0f;
        GLfloat fDif[] = { fi * 1.0f, fi * 0.65f, fi * 0.2f, 1.0f };
        glLightfv(GL_LIGHT1, GL_POSITION, fPos);
        glLightfv(GL_LIGHT1, GL_DIFFUSE,  fDif);
        glEnable(GL_LIGHT1);
    } else { glDisable(GL_LIGHT1); }
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluPerspective(60.0, (double)aspect, 0.05, 10.0);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glTranslatef(0.26f, -0.20f - w->recoilY - w->reloadY, -0.60f);
    if (isMoving) {
        float bobX = (float)(sin(t * 9.0) * 0.012);
        float bobY = (float)(fabs(sin(t * 9.0)) * 0.010);
        glTranslatef(bobX, bobY, 0.0f);
    }
    glTranslatef((float)(sin(t * 1.3) * 0.004), (float)(sin(t * 1.8) * 0.003), 0.0f);
    glRotatef(4.0f,  1.0f, 0.0f, 0.0f);
    glRotatef(-6.0f, 0.0f, 1.0f, 0.0f);
    if      (gCurrentWeapon == WEAPON_PISTOL)  renderPistolModel();
    else if (gCurrentWeapon == WEAPON_SHOTGUN) renderShotgunModel();
    else                                        renderM416Model();
    if (gMuzzleFlash > 0.0f) {
        float alpha  = gMuzzleFlash / 0.10f;
        float fzOff  = (gCurrentWeapon == WEAPON_PISTOL)  ? -0.42f :
                       (gCurrentWeapon == WEAPON_SHOTGUN) ? -0.75f : -0.65f;
        float fScale = (gCurrentWeapon == WEAPON_PISTOL)  ?  0.05f :
                       (gCurrentWeapon == WEAPON_SHOTGUN) ?  0.075f : 0.06f;
        glDisable(GL_LIGHTING); glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.0f, 0.65f, 0.15f, alpha * 0.6f);
        glPushMatrix(); glTranslatef(0.0f, 0.022f, fzOff); glutSolidSphere(fScale * 1.8f, 8, 8); glPopMatrix();
        glColor4f(1.0f, 0.95f, 0.7f, alpha);
        glPushMatrix(); glTranslatef(0.0f, 0.022f, fzOff); glutSolidSphere(fScale, 8, 8); glPopMatrix();
        glDisable(GL_BLEND); glEnable(GL_LIGHTING);
    }
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
    glDisable(GL_LIGHTING); glDisable(GL_COLOR_MATERIAL); glDisable(GL_DEPTH_TEST);
}

/* Render semua peluru yang sedang melayang di udara dengan efek cahaya dan jejak menggunakan billboarding 2D */
void renderProjectiles(Player* player) {
    int   i;
    float det = player->dirX * player->planeY - player->planeX * player->dirY;
    if (fabsf(det) < 0.00001f) det = 0.00001f;
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0, SCREEN_W, SCREEN_H, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (i = 0; i < MAX_PROJECTILES; i++) {
        Projectile* p = &gProjectiles[i];
        int   scrX, projH, halfW, drawS, drawE;
        float relX, relY;
        if (!p->active) continue;
        relX = p->x - player->x;
        relY = p->y - player->y;
        float transformX = (player->dirX * relY - player->dirY * relX) / det;
        float transformY = (player->planeY * relX - player->planeX * relY) / det;
        if (transformY <= 0.05f) continue;
        scrX  = (int)((SCREEN_W / 2) * (1.0f + transformX / transformY));
        projH = abs((int)(SCREEN_H / transformY));
        halfW = projH / 14;
        if (halfW < 2) halfW = 2;
        drawS = SCREEN_H / 2 - projH / 2;
        drawE = SCREEN_H / 2 + projH / 2;
        if (drawS < 0)         drawS = 0;
        if (drawE >= SCREEN_H) drawE = SCREEN_H - 1;
        if (scrX < 0 || scrX >= SCREEN_W) continue;
        if (transformY >= zBuffer[scrX]) continue;
        glColor4f(p->r, p->g, p->b, 0.55f);
        glBegin(GL_QUADS);
            glVertex2f((float)(scrX - halfW*2), (float)(drawS + projH*2/7));
            glVertex2f((float)(scrX + halfW*2), (float)(drawS + projH*2/7));
            glVertex2f((float)(scrX + halfW*2), (float)(drawE - projH*2/7));
            glVertex2f((float)(scrX - halfW*2), (float)(drawE - projH*2/7));
        glEnd();
        glColor4f(p->r, p->g, p->b, 0.90f);
        glBegin(GL_QUADS);
            glVertex2f((float)(scrX - halfW), (float)(drawS + projH*3/8));
            glVertex2f((float)(scrX + halfW), (float)(drawS + projH*3/8));
            glVertex2f((float)(scrX + halfW), (float)(drawE - projH*3/8));
            glVertex2f((float)(scrX - halfW), (float)(drawE - projH*3/8));
        glEnd();
        glColor4f(1.0f, 1.0f, 0.9f, 0.98f);
        glBegin(GL_QUADS);
            glVertex2f((float)(scrX - 1), (float)(drawS + projH*3/8));
            glVertex2f((float)(scrX + 1), (float)(drawS + projH*3/8));
            glVertex2f((float)(scrX + 1), (float)(drawE - projH*3/8));
            glVertex2f((float)(scrX - 1), (float)(drawE - projH*3/8));
        glEnd();
    }
    glDisable(GL_BLEND);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

const char* weaponGetName(void)       { return (gCurrentWeapon == WEAPON_NONE) ? "FISTS" : gWeapons[gCurrentWeapon].name; }
int         weaponGetAmmo(void)       { return (gCurrentWeapon == WEAPON_NONE) ? 0 : gWeapons[gCurrentWeapon].ammo; }
int         weaponGetMaxAmmo(void)    { return (gCurrentWeapon == WEAPON_NONE) ? 0 : gWeapons[gCurrentWeapon].maxAmmo; }
int         weaponGetReserveAmmo(void) { return (gCurrentWeapon == WEAPON_NONE) ? 0 : gWeapons[gCurrentWeapon].reserveAmmo; }
int         weaponIsReloading(void)   { return (gCurrentWeapon == WEAPON_NONE) ? 0 : gWeapons[gCurrentWeapon].isReloading; }
float       weaponGetReloadRatio(void) {
    Weapon* w;
    if (gCurrentWeapon == WEAPON_NONE) return 1.0f;
    w = &gWeapons[gCurrentWeapon];
    if (!w->isReloading) return 1.0f;
    return w->reloadTimer / w->reloadDuration;
}
float       weaponGetReadyRatio(void) {
    Weapon* w;
    float r;
    if (gCurrentWeapon == WEAPON_NONE) return 1.0f;
    w = &gWeapons[gCurrentWeapon];
    r   = w->timer / w->cooldown;
    return (r > 1.0f) ? 1.0f : r;
}
float       weaponGetOutOfAmmoTimer(void) { return gOutOfAmmoTimer; }

/* Membuka (unlock) senjata baru, misalnya saat pemain mengambil crate, lalu mengisi peluru secara otomatis */
void weaponUnlock(int type) {
    if (type < 0 || type >= NUM_WEAPONS) return;
    if (gWeapons[type].unlocked) return;  /* already unlocked */
    gWeapons[type].unlocked    = 1;
    gWeapons[type].ammo        = gWeapons[type].maxAmmo;
    gWeapons[type].reserveAmmo = gWeapons[type].maxReserveAmmo;
    gWeapons[type].timer       = 9999.0f;
    printf("[Weapon] Unlocked: %s (ammo %d + reserve %d)\n",
           gWeapons[type].name, gWeapons[type].ammo, gWeapons[type].reserveAmmo);
    /* Auto-equip if bare hands */
    if (gCurrentWeapon == WEAPON_NONE) {
        gCurrentWeapon = type;
    }
}
