#ifndef PLAYER_H
#define PLAYER_H

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

void playerInit(Player* p);
void playerUpdateDirection(Player* p);
void playerRotate(Player* p, float deltaAngle);
void playerPitch(Player* p, int dy);
void playerMove(Player* p, float dt);

#endif
