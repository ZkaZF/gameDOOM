#ifndef WEAPON_H
#define WEAPON_H

#include "player.h"
#include <GL/glut.h>

#define WEAPON_PISTOL   0
#define WEAPON_SHOTGUN  1
#define WEAPON_M416     2
#define WEAPON_NONE    -1
#define NUM_WEAPONS     3
#define MAX_PROJECTILES 64
#define PROJ_SPEED      0.44f
#define PROJ_MAX_DIST   18.0f

typedef struct {
    float x, y;
    float dirX, dirY;
    float dist;
    int   active;
    int   damage;
    float r, g, b;
} Projectile;

typedef struct {
    int   type;
    char  name[16];
    int   ammo;
    int   maxAmmo;
    float cooldown;
    float timer;
    float recoilY;
    float spreadAngle;
    int   pelletsPerShot;
    int   damage;
    float projR, projG, projB;
    int   isReloading;
    float reloadTimer;
    float reloadDuration;
    float reloadY;
    int   unlocked;
    int   reserveAmmo;
    int   maxReserveAmmo;
} Weapon;

extern Weapon      gWeapons[NUM_WEAPONS];
extern int         gCurrentWeapon;
extern Projectile  gProjectiles[MAX_PROJECTILES];
extern GLUquadric* gQuad;
extern float       gMuzzleFlash;

void        weaponInit(void);
void        spawnProjectile(float x, float y, float dx, float dy, int dmg, float r, float g, float b);
void        weaponShoot(Player* player);
void        weaponReload(void);
void        weaponSwitch(int index);
void        weaponUpdate(float dt);
void        weaponUnlock(int type);
void        renderWeapon3D(Player* player);
void        renderProjectiles(Player* player);
const char* weaponGetName(void);
int         weaponGetAmmo(void);
int         weaponGetMaxAmmo(void);
int         weaponIsReloading(void);
float       weaponGetReloadRatio(void);
float       weaponGetReadyRatio(void);

#endif
