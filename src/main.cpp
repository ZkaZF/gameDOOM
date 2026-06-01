
#include <GL/glut.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "map.h"
#include "texture.h"
#include "player.h"
#include "raycaster.h"
#include "weapon.h"
#include "Ompreng.h"
#include "FoodItem.h"
#include "audio.h"
#include "enemy.h"
#include "item.h"
#include "hud.h"
int gGameWon = 0;
int gShowControls = 1;
static Player player;
static int windowCenterX   = SCREEN_W / 2;
static int windowCenterY   = SCREEN_H / 2;
static int mouseWarping    = 0;
static int mouseButtonHeld = 0;
static int prevTime        = 0;
static void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, SCREEN_W, SCREEN_H, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    renderRaycastView(&player);
    renderProjectiles(&player);
    renderEnemies(&player);
    renderItems(&player);
    renderOmprengItems(&player);
    renderFoodItems(&player);
    renderWeapon3D(&player);
    drawHUD(&player);
    glutSwapBuffers();
}
static void reshape(int width, int height) {
    if (height == 0) height = 1;
    glViewport(0, 0, width, height);
    windowCenterX = width  / 2;
    windowCenterY = height / 2;
}
static void keyDown(unsigned char key, int x, int y) {
    (void)x; (void)y;
    if (key == 27) { exit(0); }
    if (gShowControls) { gShowControls = 0; return; }
    switch (key) {
        case 'w': case 'W': player.moveForward  = 1; break;
        case 's': case 'S': player.moveBackward = 1; break;
        case 'a': case 'A': player.strafeLeft   = 1; break;
        case 'd': case 'D': player.strafeRight  = 1; break;
        case '1': weaponSwitch(WEAPON_PISTOL);  break;
        case '2': weaponSwitch(WEAPON_SHOTGUN); break;
        case '3': weaponSwitch(WEAPON_M416);    break;
        case ' ':
            /* Jump — only if on ground */
            if (player.jumpZ <= 0.0f && player.jumpVel <= 0.0f) {
                player.jumpVel = JUMP_VEL;
            }
            break;
        case 'r': case 'R': weaponReload(); break;
    }
}
static void keyUp(unsigned char key, int x, int y) {
    (void)x; (void)y;
    switch (key) {
        case 'w': case 'W': player.moveForward  = 0; break;
        case 's': case 'S': player.moveBackward = 0; break;
        case 'a': case 'A': player.strafeLeft   = 0; break;
        case 'd': case 'D': player.strafeRight  = 0; break;
    }
}
static void specialKey(int key, int x, int y) {
    (void)x; (void)y;
    if (key == GLUT_KEY_F5) {
        gGameWon = 0;
        gShowControls = 1;
        audioStop();
        playerInit(&player);
        itemInit();
        omprengInit();
        foodInit();
        audioInit();
        enemyInitLevel();
    }
}
static void mouseMotion(int x, int y) {
    int dx, dy;
    if (mouseWarping) { mouseWarping = 0; return; }
    dx = x - windowCenterX;
    dy = y - windowCenterY;
    if (dx != 0) playerRotate(&player, dx * MOUSE_SENS);
    if (dy != 0) playerPitch(&player, dy);
    if (dx != 0 || dy != 0) {
        mouseWarping = 1;
        glutWarpPointer(windowCenterX, windowCenterY);
    }
}
static void mouseButton(int button, int state, int x, int y) {
    (void)x; (void)y;
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            if (gShowControls) { gShowControls = 0; return; }
            mouseButtonHeld = 1;
            weaponShoot(&player);
        } else {
            mouseButtonHeld = 0;
        }
    }
    if (button == 3) weaponSwitch((gCurrentWeapon + 1) % NUM_WEAPONS);
    if (button == 4) weaponSwitch((gCurrentWeapon - 1 + NUM_WEAPONS) % NUM_WEAPONS);
}
static void idle(void) {
    int   currTime = glutGet(GLUT_ELAPSED_TIME);
    float dt       = (float)(currTime - prevTime) / 1000.0f;
    if (dt > 0.05f) dt = 0.05f;
    prevTime = currTime;
    if (mouseButtonHeld && player.health > 0 && !gShowControls)
        weaponShoot(&player);
    if (player.health > 0 && !gShowControls) {
        /* Sprint: check Shift key */
        player.sprinting = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 1 : 0;
        playerMove(&player, dt);
        weaponUpdate(dt);
        enemyUpdate(&player, dt);
        itemUpdate(&player, dt);
        omprengUpdate(&player, dt);
        foodUpdate(&player, dt);
        /* Win condition: all items collected + boss defeated */
        if (!gGameWon && foodAllCollected() && gArenas[2].completed) {
            gGameWon = 1;
        }
    }
    if (gDamageFlash > 0.0f) {
        gDamageFlash -= dt * 3.0f;
        if (gDamageFlash < 0.0f) gDamageFlash = 0.0f;
    }
    glutPostRedisplay();
}
int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitWindowSize(SCREEN_W, SCREEN_H);
    glutInitWindowPosition(100, 50);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutCreateWindow("DOOM GTI - FPS Shooter");
    glutFullScreen();
    playerInit(&player);
    weaponInit();
    itemInit();
    omprengInit();
    foodInit();
    audioInit();
    enemyInitLevel();
    initGoblinSpriteTexture();
    initBossSpriteTexture();
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialKey);
    glutPassiveMotionFunc(mouseMotion);
    glutMotionFunc(mouseMotion);
    glutMouseFunc(mouseButton);
    glutIdleFunc(idle);
    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer(windowCenterX, windowCenterY);
    prevTime = glutGet(GLUT_ELAPSED_TIME);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_NORMALIZE);
    printf("=== DOOM GTI - Version 3 ===\n");
    printf("Controls:\n");
    printf("  WASD         - Move\n");
    printf("  Mouse X      - Look left/right\n");
    printf("  Mouse Y      - Look up/down\n");
    printf("  LMB / Space  - Shoot\n");
    printf("  R            - Reload\n");
    printf("  1 / 2        - Switch weapon\n");
    printf("  Scroll       - Switch weapon\n");
    printf("  F5           - Respawn / Restart\n");
    printf("  ESC          - Quit\n");
    printf("Enemies: 5 Imps, 2 Demons, 3 Spectres\n");
    printf("============================\n");
    glutMainLoop();
    return EXIT_SUCCESS;
}
