#ifndef WEAPON_H
#define WEAPON_H

/*
 * weapon.h — Weapon System (V2)
 *
 * Implements:
 *   - 2 weapons: Pistol (index 0) and Shotgun (index 1)
 *   - 3D weapon models using GLU/GLUT primitives
 *   - Projectile spawn, movement, wall collision
 *   - Projectile billboard rendering (with z-buffer test)
 *   - Recoil + weapon bob animation
 *   - Muzzle flash effect
 *
 * Depends on (included before this in main.cpp):
 *   map.h     -> getMap()
 *   player.h  -> Player struct
 *   raycaster.h -> zBuffer[], SCREEN_W, SCREEN_H
 */

#include <GL/glut.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* ───────────────────── Constants ───────────────────── */
#define WEAPON_PISTOL   0
#define WEAPON_SHOTGUN  1
#define WEAPON_M416     2
#define NUM_WEAPONS     3

#define MAX_PROJECTILES 64
#define PROJ_SPEED      0.22f
#define PROJ_MAX_DIST   18.0f

/* ───────────────────── Structs ───────────────────── */

typedef struct {
    float x, y;        /* 2D position in map coordinates */
    float dirX, dirY;  /* normalized direction vector */
    float dist;        /* total distance traveled */
    int   active;
    int   damage;
    float r, g, b;     /* bullet color */
} Projectile;

typedef struct {
    int   type;
    char  name[16];
    int   ammo;
    int   maxAmmo;
    float cooldown;          /* seconds between shots */
    float timer;             /* elapsed since last shot */
    float recoilY;           /* current vertical recoil offset */
    float spreadAngle;       /* half-spread in radians */
    int   pelletsPerShot;    /* 1 = pistol, 7 = shotgun */
    int   damage;            /* per-pellet damage */
    float projR, projG, projB; /* projectile color */
    /* Reload system */
    int   isReloading;       /* 1 = currently reloading */
    float reloadTimer;       /* elapsed since reload started */
    float reloadDuration;    /* total reload time (seconds) */
    float reloadY;           /* weapon dip offset during reload animation */
} Weapon;

/* ───────────────────── Globals ───────────────────── */
static Weapon      gWeapons[NUM_WEAPONS];
static int         gCurrentWeapon = WEAPON_PISTOL;
static Projectile  gProjectiles[MAX_PROJECTILES];
static GLUquadric* gQuad = NULL;
static float       gMuzzleFlash = 0.0f;

/* ───────────────────── Init ───────────────────── */
static void weaponInit(void) {
    memset(gProjectiles, 0, sizeof(gProjectiles));

    /* ── Pistol ── */
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

    /* ── Shotgun ── */
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

    /* ── M416 (Full-Auto Assault Rifle) ── */
    gWeapons[WEAPON_M416].type           = WEAPON_M416;
    strcpy(gWeapons[WEAPON_M416].name, "M416");
    gWeapons[WEAPON_M416].ammo           = 30;
    gWeapons[WEAPON_M416].maxAmmo        = 30;
    gWeapons[WEAPON_M416].cooldown       = 0.092f;   /* ~650 RPM */
    gWeapons[WEAPON_M416].timer          = 9999.0f;
    gWeapons[WEAPON_M416].recoilY        = 0.0f;
    gWeapons[WEAPON_M416].spreadAngle    = 0.025f;   /* slight spread */
    gWeapons[WEAPON_M416].pelletsPerShot = 1;
    gWeapons[WEAPON_M416].damage         = 15;
    gWeapons[WEAPON_M416].projR          = 0.55f;    /* green-ish tracer */
    gWeapons[WEAPON_M416].projG          = 0.95f;
    gWeapons[WEAPON_M416].projB          = 0.30f;
    gWeapons[WEAPON_M416].isReloading    = 0;
    gWeapons[WEAPON_M416].reloadTimer    = 0.0f;
    gWeapons[WEAPON_M416].reloadDuration = 2.4f;
    gWeapons[WEAPON_M416].reloadY        = 0.0f;

    /* GLU quadric for cylinder barrels */
    gQuad = gluNewQuadric();
    gluQuadricDrawStyle(gQuad, GLU_FILL);
    gluQuadricNormals(gQuad, GLU_SMOOTH);
}

/* ───────────────────── Projectile Spawn ───────────────────── */
static void spawnProjectile(float x, float y,
                            float dx, float dy,
                            int dmg,
                            float r, float g, float b) {
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

/* ───────────────────── Shoot ───────────────────── */
static void weaponShoot(Player* player) {
    Weapon* w = &gWeapons[gCurrentWeapon];
    int i;

    if (w->isReloading)          return; /* reloading */
    if (w->timer < w->cooldown)  return; /* still cooling down */
    if (w->ammo <= 0) {
        /* Auto-trigger reload when empty */
        if (!w->isReloading) { w->isReloading = 1; w->reloadTimer = 0.0f; }
        return;
    }

    w->ammo--;
    w->timer   = 0.0f;
    w->recoilY = (gCurrentWeapon == WEAPON_SHOTGUN) ? 0.20f :
                 (gCurrentWeapon == WEAPON_M416)    ? 0.04f : 0.10f;
    gMuzzleFlash = 0.10f;

    for (i = 0; i < w->pelletsPerShot; i++) {
        float pDirX, pDirY;

        if (w->pelletsPerShot == 1) {
            pDirX = player->dirX;
            pDirY = player->dirY;
        } else {
            /* Spread evenly from -spread to +spread */
            float t     = (w->pelletsPerShot > 1)
                          ? (float)i / (float)(w->pelletsPerShot - 1)
                          : 0.5f;
            float angle = -w->spreadAngle + t * (2.0f * w->spreadAngle);
            float cosA  = cosf(angle);
            float sinA  = sinf(angle);
            pDirX = player->dirX * cosA - player->dirY * sinA;
            pDirY = player->dirX * sinA + player->dirY * cosA;
        }

        spawnProjectile(
            player->x + pDirX * 0.35f,
            player->y + pDirY * 0.35f,
            pDirX, pDirY,
            w->damage,
            w->projR, w->projG, w->projB
        );
    }
}

/* ───────────────────── Reload ───────────────────── */
static void weaponReload(void) {
    Weapon* w = &gWeapons[gCurrentWeapon];
    if (w->isReloading) return;        /* already reloading */
    if (w->ammo >= w->maxAmmo) return; /* already full */
    w->isReloading = 1;
    w->reloadTimer = 0.0f;
}

/* ───────────────────── Switch Weapon ───────────────────── */
static void weaponSwitch(int index) {
    if (index < 0 || index >= NUM_WEAPONS) return;
    gCurrentWeapon = index;
    gWeapons[gCurrentWeapon].recoilY = 0.0f;
}

/* ───────────────────── Update (call each frame) ───────────────────── */
static void weaponUpdate(float dt) {
    int i;
    Weapon* w = &gWeapons[gCurrentWeapon];

    /* Fire cooldown */
    w->timer += dt;

    /* Recoil recovery */
    if (w->recoilY > 0.0f) {
        w->recoilY -= dt * 1.2f;
        if (w->recoilY < 0.0f) w->recoilY = 0.0f;
    }

    /* ── Reload animation ── */
    if (w->isReloading) {
        w->reloadTimer += dt;
        /* Weapon dips down during first half, rises back in second half */
        {
            float t  = w->reloadTimer / w->reloadDuration;
            float hl = 0.5f; /* half-point */
            if (t < hl)
                w->reloadY = (t / hl) * 0.45f;         /* dip down */
            else
                w->reloadY = (1.0f - (t - hl) / hl) * 0.45f; /* rise up */
            if (w->reloadY < 0.0f) w->reloadY = 0.0f;
        }

        if (w->reloadTimer >= w->reloadDuration) {
            w->ammo       = w->maxAmmo;  /* fully reloaded */
            w->isReloading = 0;
            w->reloadY     = 0.0f;
            w->reloadTimer = 0.0f;
        }
    } else {
        w->reloadY = 0.0f;
    }

    /* Muzzle flash decay */
    if (gMuzzleFlash > 0.0f) {
        gMuzzleFlash -= dt * 2.5f;
        if (gMuzzleFlash < 0.0f) gMuzzleFlash = 0.0f;
    }

    /* Projectile movement */
    for (i = 0; i < MAX_PROJECTILES; i++) {
        Projectile* p = &gProjectiles[i];
        if (!p->active) continue;

        p->x    += p->dirX * PROJ_SPEED;
        p->y    += p->dirY * PROJ_SPEED;
        p->dist += PROJ_SPEED;

        if (p->dist >= PROJ_MAX_DIST) { p->active = 0; continue; }
        if (getMap((int)p->x, (int)p->y) > 0) { p->active = 0; continue; }
    }
}

/* ───────────────────── 3D Weapon Model: Pistol ───────────────────── */
static void renderPistolModel(void) {
    /* Slide / body */
    glColor3f(0.22f, 0.22f, 0.26f);
    glPushMatrix();
        glScalef(0.11f, 0.08f, 0.30f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Barrel */
    glColor3f(0.16f, 0.16f, 0.18f);
    glPushMatrix();
        glTranslatef(0.0f, 0.016f, -0.22f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(gQuad, 0.024f, 0.021f, 0.20f, 10, 1);
    glPopMatrix();

    /* Grip (dark wood) */
    glColor3f(0.28f, 0.17f, 0.08f);
    glPushMatrix();
        glTranslatef(0.0f, -0.092f, 0.04f);
        glRotatef(14.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.09f, 0.16f, 0.10f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Trigger guard */
    glColor3f(0.20f, 0.20f, 0.23f);
    glPushMatrix();
        glTranslatef(0.0f, -0.052f, 0.02f);
        glScalef(0.075f, 0.035f, 0.12f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Front sight */
    glColor3f(0.9f, 0.9f, 0.9f);
    glPushMatrix();
        glTranslatef(0.0f, 0.050f, -0.12f);
        glScalef(0.014f, 0.020f, 0.018f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Rear sight notch */
    glColor3f(0.9f, 0.9f, 0.9f);
    glPushMatrix();
        glTranslatef(-0.022f, 0.048f, 0.10f);
        glScalef(0.012f, 0.018f, 0.014f);
        glutSolidCube(1.0f);
    glPopMatrix();
    glPushMatrix();
        glTranslatef( 0.022f, 0.048f, 0.10f);
        glScalef(0.012f, 0.018f, 0.014f);
        glutSolidCube(1.0f);
    glPopMatrix();
}

/* ───────────────────── 3D Weapon Model: Shotgun ───────────────────── */
static void renderShotgunModel(void) {
    /* Receiver / body */
    glColor3f(0.26f, 0.20f, 0.13f);
    glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.06f);
        glScalef(0.17f, 0.13f, 0.48f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Barrel Left */
    glColor3f(0.18f, 0.18f, 0.20f);
    glPushMatrix();
        glTranslatef(-0.038f, 0.022f, -0.30f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(gQuad, 0.028f, 0.025f, 0.45f, 10, 1);
    glPopMatrix();

    /* Barrel Right */
    glPushMatrix();
        glTranslatef( 0.038f, 0.022f, -0.30f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(gQuad, 0.028f, 0.025f, 0.45f, 10, 1);
    glPopMatrix();

    /* Fore-end / pump */
    glColor3f(0.33f, 0.26f, 0.16f);
    glPushMatrix();
        glTranslatef(0.0f, 0.002f, -0.14f);
        glScalef(0.16f, 0.11f, 0.16f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Stock */
    glColor3f(0.30f, 0.22f, 0.12f);
    glPushMatrix();
        glTranslatef(0.0f, -0.028f, 0.32f);
        glRotatef(-7.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.14f, 0.15f, 0.24f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Trigger guard */
    glColor3f(0.18f, 0.18f, 0.20f);
    glPushMatrix();
        glTranslatef(0.0f, -0.075f, 0.06f);
        glScalef(0.07f, 0.04f, 0.16f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Sight bead (golden) */
    glColor3f(0.90f, 0.75f, 0.10f);
    glPushMatrix();
        glTranslatef(0.0f, 0.068f, -0.27f);
        glutSolidSphere(0.013f, 8, 6);
    glPopMatrix();

    /* Shell ejection port detail */
    glColor3f(0.15f, 0.15f, 0.17f);
    glPushMatrix();
        glTranslatef(0.09f, 0.04f, 0.04f);
        glScalef(0.02f, 0.06f, 0.10f);
        glutSolidCube(1.0f);
    glPopMatrix();
}

/* ────────────────────────────── 3D Weapon Model: M416 ────────────────────────────── */
static void renderM416Model(void) {
    /* Receiver — main body */
    glColor3f(0.20f, 0.22f, 0.20f);
    glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.0f);
        glScalef(0.10f, 0.09f, 0.48f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Barrel — long and thin */
    glColor3f(0.15f, 0.16f, 0.15f);
    glPushMatrix();
        glTranslatef(0.0f, 0.012f, -0.38f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(gQuad, 0.018f, 0.016f, 0.32f, 10, 1);
    glPopMatrix();

    /* Gas tube (above barrel) */
    glColor3f(0.18f, 0.19f, 0.17f);
    glPushMatrix();
        glTranslatef(0.0f, 0.038f, -0.30f);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(gQuad, 0.009f, 0.009f, 0.26f, 8, 1);
    glPopMatrix();

    /* Handguard (around barrel, slightly wider) */
    glColor3f(0.25f, 0.27f, 0.24f);
    glPushMatrix();
        glTranslatef(0.0f, 0.006f, -0.22f);
        glScalef(0.13f, 0.10f, 0.25f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Magazine (dark, angled slightly forward) */
    glColor3f(0.14f, 0.15f, 0.13f);
    glPushMatrix();
        glTranslatef(0.0f, -0.13f, 0.04f);
        glRotatef(7.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.07f, 0.20f, 0.10f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Pistol grip */
    glColor3f(0.18f, 0.14f, 0.10f);
    glPushMatrix();
        glTranslatef(0.0f, -0.10f, 0.15f);
        glRotatef(18.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.07f, 0.15f, 0.09f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Stock — collapsible style */
    glColor3f(0.20f, 0.22f, 0.20f);
    glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.30f);
        glScalef(0.08f, 0.06f, 0.20f);
        glutSolidCube(1.0f);
    glPopMatrix();
    /* Stock butt plate */
    glColor3f(0.12f, 0.12f, 0.12f);
    glPushMatrix();
        glTranslatef(0.0f, -0.018f, 0.41f);
        glScalef(0.09f, 0.09f, 0.04f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Carry handle / sight rail */
    glColor3f(0.22f, 0.24f, 0.22f);
    glPushMatrix();
        glTranslatef(0.0f, 0.058f, 0.05f);
        glScalef(0.06f, 0.04f, 0.30f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Front sight post */
    glColor3f(0.90f, 0.90f, 0.88f);
    glPushMatrix();
        glTranslatef(0.0f, 0.076f, -0.17f);
        glScalef(0.010f, 0.022f, 0.012f);
        glutSolidCube(1.0f);
    glPopMatrix();

    /* Charging handle (right side) */
    glColor3f(0.15f, 0.16f, 0.15f);
    glPushMatrix();
        glTranslatef(0.07f, 0.008f, 0.11f);
        glScalef(0.025f, 0.025f, 0.06f);
        glutSolidCube(1.0f);
    glPopMatrix();
}

/* ───────────────────── Render 3D Weapon (floating in screen space) ─── */
static void renderWeapon3D(Player* player) {
    Weapon* w    = &gWeapons[gCurrentWeapon];
    float aspect = (float)SCREEN_W / (float)SCREEN_H;
    double t     = (double)glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    int isMoving = player->moveForward || player->moveBackward ||
                   player->strafeLeft  || player->strafeRight;

    /* Clear depth so weapon always draws on top of raycasted scene */
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    /* ── Main light ── */
    {
        GLfloat lPos[] = { 1.0f, 3.0f, 1.5f, 0.0f };
        GLfloat lAmb[] = { 0.35f, 0.32f, 0.28f, 1.0f };
        GLfloat lDif[] = { 0.85f, 0.80f, 0.70f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, lPos);
        glLightfv(GL_LIGHT0, GL_AMBIENT,  lAmb);
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  lDif);
    }

    /* ── Muzzle flash light (LIGHT1) ── */
    if (gMuzzleFlash > 0.0f) {
        GLfloat fPos[] = { 0.0f, 0.0f, -1.0f, 1.0f };
        float   fi     = gMuzzleFlash * 10.0f;
        GLfloat fDif[] = { fi * 1.0f, fi * 0.65f, fi * 0.2f, 1.0f };
        glLightfv(GL_LIGHT1, GL_POSITION, fPos);
        glLightfv(GL_LIGHT1, GL_DIFFUSE,  fDif);
        glEnable(GL_LIGHT1);
    } else {
        glDisable(GL_LIGHT1);
    }

    /* ── 3D perspective projection ── */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(60.0, (double)aspect, 0.05, 10.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    /* Base weapon position: right-center-bottom of view */
    glTranslatef(0.26f, -0.20f - w->recoilY - w->reloadY, -0.60f);

    /* Walking bob animation */
    if (isMoving) {
        float bobX = (float)(sin(t * 9.0) * 0.012);
        float bobY = (float)(fabs(sin(t * 9.0)) * 0.010);
        glTranslatef(bobX, bobY, 0.0f);
    }

    /* Idle sway (very subtle) */
    glTranslatef(
        (float)(sin(t * 1.3) * 0.004),
        (float)(sin(t * 1.8) * 0.003),
        0.0f
    );

    /* Tilt slightly toward player for natural look */
    glRotatef(4.0f,  1.0f, 0.0f, 0.0f);
    glRotatef(-6.0f, 0.0f, 1.0f, 0.0f);

    /* ── Render weapon model ── */
    if (gCurrentWeapon == WEAPON_PISTOL)
        renderPistolModel();
    else if (gCurrentWeapon == WEAPON_SHOTGUN)
        renderShotgunModel();
    else
        renderM416Model();

    /* ── Muzzle flash sphere ── */
    if (gMuzzleFlash > 0.0f) {
        float alpha  = gMuzzleFlash / 0.10f;
        float fzOff  = (gCurrentWeapon == WEAPON_PISTOL)  ? -0.42f :
                       (gCurrentWeapon == WEAPON_SHOTGUN) ? -0.75f : -0.65f;
        float fxOff  = 0.0f;
        float fScale = (gCurrentWeapon == WEAPON_PISTOL)  ?  0.05f :
                       (gCurrentWeapon == WEAPON_SHOTGUN) ?  0.075f : 0.06f;

        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        /* Outer glow */
        glColor4f(1.0f, 0.65f, 0.15f, alpha * 0.6f);
        glPushMatrix();
            glTranslatef(fxOff, 0.022f, fzOff);
            glutSolidSphere(fScale * 1.8f, 8, 8);
        glPopMatrix();

        /* Core */
        glColor4f(1.0f, 0.95f, 0.7f, alpha);
        glPushMatrix();
            glTranslatef(fxOff, 0.022f, fzOff);
            glutSolidSphere(fScale, 8, 8);
        glPopMatrix();

        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    /* ── Restore state ── */
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_DEPTH_TEST);
}

/* ───────────────────── Render Projectiles as Billboards ─────────────── */
static void renderProjectiles(Player* player) {
    int   i;
    float det = player->dirX * player->planeY - player->planeX * player->dirY;
    if (fabsf(det) < 0.00001f) det = 0.00001f;

    /* 2D ortho — same coordinate space as raycaster */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, SCREEN_W, SCREEN_H, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (i = 0; i < MAX_PROJECTILES; i++) {
        Projectile* p = &gProjectiles[i];
        int   scrX, projH, halfW, drawS, drawE;
        float relX, relY, tX, tY;

        if (!p->active) continue;

        relX = p->x - player->x;
        relY = p->y - player->y;

        /* Correct sprite camera transform (Lode's formula):
         *   transformX (horizontal) = (dirX*relY - dirY*relX) / det
         *   transformY (depth)      = (planeY*relX - planeX*relY) / det
         */
        float transformX = (player->dirX * relY - player->dirY * relX) / det;
        float transformY = (player->planeY * relX - player->planeX * relY) / det;

        if (transformY <= 0.05f) continue; /* behind camera */

        scrX  = (int)((SCREEN_W / 2) * (1.0f + transformX / transformY));
        projH = abs((int)(SCREEN_H / transformY));
        halfW = projH / 14;
        if (halfW < 2) halfW = 2;

        drawS = SCREEN_H / 2 - projH / 2;
        drawE = SCREEN_H / 2 + projH / 2;
        if (drawS < 0)          drawS = 0;
        if (drawE >= SCREEN_H)  drawE = SCREEN_H - 1;

        /* Z-buffer test: only draw if in front of wall */
        if (scrX < 0 || scrX >= SCREEN_W) continue;
        if (transformY >= zBuffer[scrX]) continue;

        /* Outer glow */
        glColor4f(p->r, p->g, p->b, 0.55f);
        glBegin(GL_QUADS);
            glVertex2f((float)(scrX - halfW * 2), (float)(drawS + projH * 2 / 7));
            glVertex2f((float)(scrX + halfW * 2), (float)(drawS + projH * 2 / 7));
            glVertex2f((float)(scrX + halfW * 2), (float)(drawE - projH * 2 / 7));
            glVertex2f((float)(scrX - halfW * 2), (float)(drawE - projH * 2 / 7));
        glEnd();

        /* Bullet body */
        glColor4f(p->r, p->g, p->b, 0.90f);
        glBegin(GL_QUADS);
            glVertex2f((float)(scrX - halfW), (float)(drawS + projH * 3 / 8));
            glVertex2f((float)(scrX + halfW), (float)(drawS + projH * 3 / 8));
            glVertex2f((float)(scrX + halfW), (float)(drawE - projH * 3 / 8));
            glVertex2f((float)(scrX - halfW), (float)(drawE - projH * 3 / 8));
        glEnd();

        /* Bright core */
        glColor4f(1.0f, 1.0f, 0.9f, 0.98f);
        glBegin(GL_QUADS);
            glVertex2f((float)(scrX - 1), (float)(drawS + projH * 3 / 8));
            glVertex2f((float)(scrX + 1), (float)(drawS + projH * 3 / 8));
            glVertex2f((float)(scrX + 1), (float)(drawE - projH * 3 / 8));
            glVertex2f((float)(scrX - 1), (float)(drawE - projH * 3 / 8));
        glEnd();
    }

    glDisable(GL_BLEND);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

/* ───────────────────── HUD Getters ───────────────────── */
static const char* weaponGetName(void)    { return gWeapons[gCurrentWeapon].name; }
static int         weaponGetAmmo(void)    { return gWeapons[gCurrentWeapon].ammo; }
static int         weaponGetMaxAmmo(void) { return gWeapons[gCurrentWeapon].maxAmmo; }
static int         weaponIsReloading(void){ return gWeapons[gCurrentWeapon].isReloading; }
static float       weaponGetReloadRatio(void) {
    Weapon* w = &gWeapons[gCurrentWeapon];
    if (!w->isReloading) return 1.0f;
    return w->reloadTimer / w->reloadDuration;
}
static float       weaponGetReadyRatio(void) {
    Weapon* w = &gWeapons[gCurrentWeapon];
    float r   = w->timer / w->cooldown;
    return (r > 1.0f) ? 1.0f : r;
}

#endif /* WEAPON_H */
