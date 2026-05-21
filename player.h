#ifndef PLAYER_H
#define PLAYER_H
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define MOVE_SPEED    0.17f   
#define ROT_SPEED     0.03f
#define MOUSE_SENS    0.003f
#define MOUSE_SENS_Y  0.30f  
#define PITCH_MAX     180.0f 
#define PLAYER_RADIUS 0.2f   
typedef struct {
    float x, y;        
    float angle;        
    float dirX, dirY;   
    float planeX, planeY; 
    float pitch;        
    int health;
    int armor;
    int alive;
    int moveForward;
    int moveBackward;
    int strafeLeft;
    int strafeRight;
} Player;
static void playerInit(Player* p) {
    p->x = PLAYER_START_X;
    p->y = PLAYER_START_Y;
    p->angle = PLAYER_START_ANGLE;
    p->dirX = cosf(p->angle);
    p->dirY = sinf(p->angle);
    p->planeX = -p->dirY * 0.90f;
    p->planeY =  p->dirX * 0.90f;
    p->pitch = 0.0f;
    p->health = 100;
    p->armor = 0;
    p->alive = 1;
    p->moveForward = 0;
    p->moveBackward = 0;
    p->strafeLeft = 0;
    p->strafeRight = 0;
}
static void playerUpdateDirection(Player* p) {
    p->dirX = cosf(p->angle);
    p->dirY = sinf(p->angle);
    p->planeX = -p->dirY * 0.90f;
    p->planeY =  p->dirX * 0.90f;
}
static void playerRotate(Player* p, float deltaAngle) {
    p->angle += deltaAngle;
    while (p->angle < 0)       p->angle += 2.0f * (float)M_PI;
    while (p->angle >= 2.0f * (float)M_PI) p->angle -= 2.0f * (float)M_PI;
    playerUpdateDirection(p);
}
static void playerPitch(Player* p, int dy) {
    p->pitch -= dy * MOUSE_SENS_Y;
    if (p->pitch >  PITCH_MAX) p->pitch =  PITCH_MAX;
    if (p->pitch < -PITCH_MAX) p->pitch = -PITCH_MAX;
}
static void playerMove(Player* p, float dt) {
    float spd = MOVE_SPEED * dt * 60.0f;
    float moveX = 0, moveY = 0;
    float newX, newY;
    if (p->moveForward) {
        moveX += p->dirX * spd;
        moveY += p->dirY * spd;
    }
    if (p->moveBackward) {
        moveX -= p->dirX * spd;
        moveY -= p->dirY * spd;
    }
    if (p->strafeLeft) {
        moveX -= p->planeX * spd;
        moveY -= p->planeY * spd;
    }
    if (p->strafeRight) {
        moveX += p->planeX * spd;
        moveY += p->planeY * spd;
    }
    newX = p->x + moveX;
    newY = p->y + moveY;
    if (isWalkable(newX + PLAYER_RADIUS * (moveX > 0 ? 1 : -1), p->y)) {
        p->x = newX;
    }
    if (isWalkable(p->x, newY + PLAYER_RADIUS * (moveY > 0 ? 1 : -1))) {
        p->y = newY;
    }
}
#endif 
