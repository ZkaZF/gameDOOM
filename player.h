#ifndef PLAYER_H
#define PLAYER_H

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ───────────────────── Player Constants ───────────────────── */
#define MOVE_SPEED    0.05f
#define ROT_SPEED     0.03f
#define MOUSE_SENS    0.003f
#define MOUSE_SENS_Y  0.30f  /* vertical look sensitivity (pixels per mouse unit) */
#define PITCH_MAX     180.0f /* max vertical offset in pixels */
#define PLAYER_RADIUS 0.2f   /* collision radius */

/* ───────────────────── Player State ───────────────────── */
typedef struct {
    float x, y;        /* position in map coordinates */
    float angle;        /* viewing angle in radians */
    float dirX, dirY;   /* direction vector (derived from angle) */
    float planeX, planeY; /* camera plane (perpendicular to dir, scaled by FOV) */
    float pitch;        /* vertical look offset in pixels (+up / -down) */

    int health;
    int armor;
    int alive;

    /* movement flags (set by keyboard input) */
    int moveForward;
    int moveBackward;
    int strafeLeft;
    int strafeRight;
} Player;

/* Initialize player at starting position */
static void playerInit(Player* p) {
    p->x = PLAYER_START_X;
    p->y = PLAYER_START_Y;
    p->angle = PLAYER_START_ANGLE;

    /* Calculate direction from angle */
    p->dirX = cosf(p->angle);
    p->dirY = sinf(p->angle);

    /* Camera plane perpendicular to direction, FOV ~66 degrees */
    /* plane length = tan(FOV/2) = tan(33deg) ≈ 0.66 */
    p->planeX = -p->dirY * 0.66f;
    p->planeY =  p->dirX * 0.66f;

    p->pitch = 0.0f;
    p->health = 100;
    p->armor = 0;
    p->alive = 1;

    p->moveForward = 0;
    p->moveBackward = 0;
    p->strafeLeft = 0;
    p->strafeRight = 0;
}

/* Update direction and camera plane from current angle */
static void playerUpdateDirection(Player* p) {
    p->dirX = cosf(p->angle);
    p->dirY = sinf(p->angle);
    p->planeX = -p->dirY * 0.66f;
    p->planeY =  p->dirX * 0.66f;
}

/* Rotate player by delta angle (from mouse movement) */
static void playerRotate(Player* p, float deltaAngle) {
    p->angle += deltaAngle;
    while (p->angle < 0)       p->angle += 2.0f * (float)M_PI;
    while (p->angle >= 2.0f * (float)M_PI) p->angle -= 2.0f * (float)M_PI;
    playerUpdateDirection(p);
}

/* Adjust vertical pitch (dy = raw mouse Y delta, positive = move down) */
static void playerPitch(Player* p, int dy) {
    p->pitch -= dy * MOUSE_SENS_Y;
    if (p->pitch >  PITCH_MAX) p->pitch =  PITCH_MAX;
    if (p->pitch < -PITCH_MAX) p->pitch = -PITCH_MAX;
}

/* Move player with collision detection */
static void playerMove(Player* p) {
    float moveX = 0, moveY = 0;
    float newX, newY;

    if (p->moveForward) {
        moveX += p->dirX * MOVE_SPEED;
        moveY += p->dirY * MOVE_SPEED;
    }
    if (p->moveBackward) {
        moveX -= p->dirX * MOVE_SPEED;
        moveY -= p->dirY * MOVE_SPEED;
    }
    if (p->strafeLeft) {
        moveX -= p->planeX * MOVE_SPEED;
        moveY -= p->planeY * MOVE_SPEED;
    }
    if (p->strafeRight) {
        moveX += p->planeX * MOVE_SPEED;
        moveY += p->planeY * MOVE_SPEED;
    }

    /* Apply movement with collision — check X and Y separately for wall sliding */
    newX = p->x + moveX;
    newY = p->y + moveY;

    /* Check X movement */
    if (isWalkable(newX + PLAYER_RADIUS * (moveX > 0 ? 1 : -1), p->y)) {
        p->x = newX;
    }
    /* Check Y movement */
    if (isWalkable(p->x, newY + PLAYER_RADIUS * (moveY > 0 ? 1 : -1))) {
        p->y = newY;
    }
}

#endif /* PLAYER_H */
