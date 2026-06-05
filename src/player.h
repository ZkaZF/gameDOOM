#ifndef PLAYER_H
#define PLAYER_H

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MOVE_SPEED    0.17f
#define SPRINT_MULT   1.85f
#define ROT_SPEED     0.03f
#define MOUSE_SENS    0.003f
#define MOUSE_SENS_Y  0.30f
#define PITCH_MAX     180.0f
#define PLAYER_RADIUS 0.2f
#define JUMP_VEL      0.25f
#define GRAVITY       0.012f
#define ADS_FOV_SCALE   0.45f
#define ADS_SPEED_SCALE 0.50f
#define ADS_LERP_SPEED  8.0f

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
    int sprinting;
    float jumpVel;
    float jumpZ;     /* vertical offset for jump */
    int   isADS;     /* 0 = hip, 1 = aim down sight (M416 only) */
    float adsFov;    /* lerp 0.0=hip .. 1.0=full ADS */
    float footstepTimer; /* throttle footstep SFX */
} Player;

void playerInit(Player* p);
void playerUpdateDirection(Player* p);
void playerRotate(Player* p, float deltaAngle);
void playerPitch(Player* p, int dy);
void playerMove(Player* p, float dt);

extern float gAdsZoom;  /* 1.0 = hip fire, >1 = zoomed (ADS) */

#endif
