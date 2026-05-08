/*
 * DOOM-Style FPS Game — Version 3: Enemies
 *
 * Tugas GTI — Semester 4
 *
 * V3 additions:
 *   - 3 enemy types: Imp (red), Demon (green tank), Spectre (blue ghost)
 *   - AI state machine: IDLE → CHASE → ATTACK → DYING → DEAD
 *   - Projectile ↔ enemy hit detection
 *   - Damage flash when player is hit
 *   - Kill counter + enemy remaining in HUD
 *   - Enemy dots on minimap
 *   - "YOU DIED" game over screen
 *
 * Include order (matters for symbol availability):
 *   map.h → texture.h → player.h → raycaster.h → weapon.h → enemy.h → hud.h
 */

#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

/* ─── Game Headers ─── */
#include "map.h"
#include "texture.h"
#include "player.h"
#include "raycaster.h"`
#include "weapon.h"
#include "enemy.h"
#include "item.h"
#include "hud.h"

/* ───────────────────── Globals ───────────────────── */
static Player player;
static int windowCenterX   = SCREEN_W / 2;
static int windowCenterY   = SCREEN_H / 2;
static int mouseWarping    = 0;
static int mouseButtonHeld = 0;
static int prevTime        = 0;

/* ───────────────────── Display ───────────────────── */
static void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* ── 2D ortho for raycasting ── */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, SCREEN_W, SCREEN_H, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    /* 1. Raycasted world (walls, floor, ceiling) */
    renderRaycastView(&player);

    /* 2. Flying projectiles (2D billboard / z-buffer) */
    renderProjectiles(&player);

    /* 3. 3D enemies (perspective pass, frustum-shifted for pitch) */
    renderEnemies(&player);

    /* 4. 3D items (rotating pickups, same frustum pass) */
    renderItems(&player);

    /* 5. 3D weapon (always on top — clears depth) */
    renderWeapon3D(&player);

    /* 6. HUD overlay */
    drawHUD(&player);

    glutSwapBuffers();
}

/* ───────────────────── Reshape ───────────────────── */
static void reshape(int width, int height) {
    if (height == 0) height = 1;
    glViewport(0, 0, width, height);
    windowCenterX = width  / 2;
    windowCenterY = height / 2;
}

/* ───────────────────── Keyboard ───────────────────── */
static void keyDown(unsigned char key, int x, int y) {
    (void)x; (void)y;
    switch (key) {
        case 27: exit(0); break;
        case 'w': case 'W': player.moveForward  = 1; break;
        case 's': case 'S': player.moveBackward = 1; break;
        case 'a': case 'A': player.strafeLeft   = 1; break;
        case 'd': case 'D': player.strafeRight  = 1; break;
        case '1': weaponSwitch(WEAPON_PISTOL);  break;
        case '2': weaponSwitch(WEAPON_SHOTGUN); break;
        case ' ': weaponShoot(&player); break;
        /* R = Reload */
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

/* Special keys (F-keys, arrow keys) */
static void specialKey(int key, int x, int y) {
    (void)x; (void)y;
    if (key == GLUT_KEY_F5) {
        /* F5 = Respawn / Restart */
        playerInit(&player);
        itemInit();
        enemyInitLevel();
    }
}

/* ───────────────────── Mouse Look (horizontal + vertical) ───────────────────── */
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

/* ───────────────────── Mouse Button ───────────────────── */
static void mouseButton(int button, int state, int x, int y) {
    (void)x; (void)y;
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            mouseButtonHeld = 1;
            weaponShoot(&player);
        } else {
            mouseButtonHeld = 0;
        }
    }
    if (button == 3) weaponSwitch((gCurrentWeapon + 1) % NUM_WEAPONS);
    if (button == 4) weaponSwitch((gCurrentWeapon - 1 + NUM_WEAPONS) % NUM_WEAPONS);
}

/* ───────────────────── Idle / Game Loop ───────────────────── */
static void idle(void) {
    int   currTime = glutGet(GLUT_ELAPSED_TIME);
    float dt       = (float)(currTime - prevTime) / 1000.0f;
    if (dt > 0.05f) dt = 0.05f;
    prevTime = currTime;

    /* Continuous fire */
    if (mouseButtonHeld && player.health > 0)
        weaponShoot(&player);

    /* Update subsystems */
    if (player.health > 0) {
        playerMove(&player);
        weaponUpdate(dt);
        enemyUpdate(&player, dt);
        itemUpdate(&player, dt);
    }

    /* Fade damage flash */
    if (gDamageFlash > 0.0f) {
        gDamageFlash -= dt * 3.0f;
        if (gDamageFlash < 0.0f) gDamageFlash = 0.0f;
    }

    glutPostRedisplay();
}

/* ───────────────────── Main ───────────────────── */
int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitWindowSize(SCREEN_W, SCREEN_H);
    glutInitWindowPosition(100, 50);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);

    glutCreateWindow("DOOM GTI - FPS Shooter");

    /* Init subsystems */
    playerInit(&player);
    weaponInit();
    itemInit();
    enemyInitLevel();

    /* Callbacks */
    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialKey);
    glutPassiveMotionFunc(mouseMotion);
    glutMotionFunc(mouseMotion);
    glutMouseFunc(mouseButton);
    glutIdleFunc(idle);

    /* Capture mouse */
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
