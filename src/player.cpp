#include "player.h"
#include "map.h"
#include "audio.h"
#include <math.h>

/* Base camera plane length (FOV reference) */
static float gBasePlaneLen = 0.90f;

/* ADS zoom factor — read by raycaster for proportional vertical scaling */
float gAdsZoom = 1.0f;

/* Menginisialisasi posisi, arah pandang, dan status (HP, ADS) player saat mulai atau respawn */
void playerInit(Player* p) {
    p->x = PLAYER_START_X;
    p->y = PLAYER_START_Y;
    p->angle = PLAYER_START_ANGLE;
    p->dirX = cosf(p->angle);
    p->dirY = sinf(p->angle);
    p->planeX = -p->dirY * gBasePlaneLen;
    p->planeY =  p->dirX * gBasePlaneLen;
    p->pitch = 0.0f;
    p->health = 100;
    p->armor = 0;
    p->alive = 1;
    p->moveForward = 0;
    p->moveBackward = 0;
    p->strafeLeft = 0;
    p->strafeRight = 0;
    p->sprinting = 0;
    p->jumpVel = 0.0f;
    p->jumpZ = 0.0f;
    p->isADS = 0;
    p->adsFov = 0.0f;
    p->footstepTimer = 0.0f;
}

/* Memperbarui vektor arah (dirX, dirY) dan camera plane (planeX, planeY) berdasarkan angle dan status zoom ADS */
void playerUpdateDirection(Player* p) {
    float fovScale = 1.0f - p->adsFov * (1.0f - ADS_FOV_SCALE);
    gAdsZoom = (fovScale > 0.001f) ? (1.0f / fovScale) : 1.0f;
    p->dirX = cosf(p->angle);
    p->dirY = sinf(p->angle);
    p->planeX = -p->dirY * gBasePlaneLen * fovScale;
    p->planeY =  p->dirX * gBasePlaneLen * fovScale;
}

/* Memutar arah pandang player secara horizontal sebesar deltaAngle */
void playerRotate(Player* p, float deltaAngle) {
    p->angle += deltaAngle;
    while (p->angle < 0)                     p->angle += 2.0f * (float)M_PI;
    while (p->angle >= 2.0f * (float)M_PI)  p->angle -= 2.0f * (float)M_PI;
    playerUpdateDirection(p);
}

/* Mengubah sudut pandang vertikal (pitch) player (melihat ke atas/bawah) */
void playerPitch(Player* p, int dy) {
    p->pitch -= dy * MOUSE_SENS_Y;
    if (p->pitch >  PITCH_MAX) p->pitch =  PITCH_MAX;
    if (p->pitch < -PITCH_MAX) p->pitch = -PITCH_MAX;
}

/* Mengatur pergerakan player (WASD), collision detection dengan tembok, lompatan, serta SFX langkah kaki */
void playerMove(Player* p, float dt) {
    float spdMult = p->sprinting ? SPRINT_MULT : 1.0f;
    /* ADS slows movement */
    if (p->isADS) spdMult *= ADS_SPEED_SCALE;
    float spd = MOVE_SPEED * spdMult * dt * 60.0f;
    float moveX = 0, moveY = 0;
    float newX, newY;
    int moving = 0;
    if (p->moveForward)  { moveX += p->dirX * spd;   moveY += p->dirY * spd;   moving = 1; }
    if (p->moveBackward) { moveX -= p->dirX * spd;   moveY -= p->dirY * spd;   moving = 1; }
    if (p->strafeLeft)   { moveX -= p->planeX * spd; moveY -= p->planeY * spd; moving = 1; }
    if (p->strafeRight)  { moveX += p->planeX * spd; moveY += p->planeY * spd; moving = 1; }
    newX = p->x + moveX;
    newY = p->y + moveY;
    if (isWalkable(newX + PLAYER_RADIUS * (moveX > 0 ? 1 : -1), p->y))
        p->x = newX;
    if (isWalkable(p->x, newY + PLAYER_RADIUS * (moveY > 0 ? 1 : -1)))
        p->y = newY;
    /* Jump physics */
    if (p->jumpZ > 0.0f || p->jumpVel > 0.0f) {
        p->jumpZ += p->jumpVel * dt * 60.0f;
        p->jumpVel -= GRAVITY * dt * 60.0f;
        if (p->jumpZ <= 0.0f) {
            p->jumpZ = 0.0f;
            p->jumpVel = 0.0f;
        }
    }
    /* Footstep SFX — throttled, only when on ground */
    if (moving && p->jumpZ <= 0.0f) {
        p->footstepTimer -= dt;
        if (p->footstepTimer <= 0.0f) {
            sfxFootstep();
            float interval = p->sprinting ? 0.28f : 0.42f;
            p->footstepTimer = interval;
        }
    } else if (!moving) {
        p->footstepTimer = 0.0f; /* reset so next step plays immediately */
    }
    /* ADS FOV smooth lerp */
    {
        float target = p->isADS ? 1.0f : 0.0f;
        float spd2 = ADS_LERP_SPEED * dt;
        if (p->adsFov < target) {
            p->adsFov += spd2;
            if (p->adsFov > target) p->adsFov = target;
        } else {
            p->adsFov -= spd2;
            if (p->adsFov < target) p->adsFov = target;
        }
        playerUpdateDirection(p);
    }
}
