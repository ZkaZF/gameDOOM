
#include "audio.h"
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "winmm.lib")

static int gAudioReady = 0;
static int gCurrentBGM = -1;  /* -1 = none, 0 = bahlil, 2 = prabowo */
static char gNowPlaying[128] = "";

static char gBahlilAbs[MAX_PATH]  = "";
static char gPrabowoAbs[MAX_PATH] = "";

static void closeCurrentBGM(void) {
    mciSendStringA("stop bgm", NULL, 0, NULL);
    mciSendStringA("close bgm", NULL, 0, NULL);
    Sleep(50); /* Give MCI time to fully release the device */
}

static int openAndPlay(const char* absPath, const char* alias, int fromMs, int toMs, int repeat) {
    char cmd[600];
    char errBuf[256];
    MCIERROR err;

    /* Try mpegvideo */
    sprintf(cmd, "open \"%s\" type mpegvideo alias %s", absPath, alias);
    err = mciSendStringA(cmd, NULL, 0, NULL);
    if (err != 0) {
        mciGetErrorStringA(err, errBuf, sizeof(errBuf));
        printf("[Audio] mpegvideo err: %s\n", errBuf);
        /* Retry without type */
        sprintf(cmd, "open \"%s\" alias %s", absPath, alias);
        err = mciSendStringA(cmd, NULL, 0, NULL);
        if (err != 0) {
            mciGetErrorStringA(err, errBuf, sizeof(errBuf));
            printf("[Audio] auto err: %s\n", errBuf);
            return 0;
        }
    }

    if (fromMs > 0 || toMs > 0) {
        if (toMs > 0) {
            sprintf(cmd, "play %s from %d to %d", alias, fromMs, toMs);
        } else {
            sprintf(cmd, "seek %s to %d", alias, fromMs);
            mciSendStringA(cmd, NULL, 0, NULL);
            if (repeat) sprintf(cmd, "play %s repeat", alias);
            else        sprintf(cmd, "play %s", alias);
        }
    } else {
        if (repeat) sprintf(cmd, "play %s repeat", alias);
        else        sprintf(cmd, "play %s", alias);
    }

    err = mciSendStringA(cmd, NULL, 0, NULL);
    if (err != 0) {
        mciGetErrorStringA(err, errBuf, sizeof(errBuf));
        printf("[Audio] play err: %s\n", errBuf);
    }
    return 1;
}

void audioInit(void) {
    GetFullPathNameA("assets/mbg-mas-bahlil-ganteng.mp3", MAX_PATH, gBahlilAbs, NULL);
    GetFullPathNameA("assets/prabowo_clean.mp3", MAX_PATH, gPrabowoAbs, NULL);

    /* Verify files exist */
    {
        DWORD attr;
        attr = GetFileAttributesA(gBahlilAbs);
        printf("[Audio] Bahlil: %s [%s]\n", gBahlilAbs, (attr != INVALID_FILE_ATTRIBUTES) ? "EXISTS" : "MISSING");
        attr = GetFileAttributesA(gPrabowoAbs);
        printf("[Audio] Prabowo: %s [%s]\n", gPrabowoAbs, (attr != INVALID_FILE_ATTRIBUTES) ? "EXISTS" : "MISSING");
    }

    closeCurrentBGM();
    gAudioReady = 1;
    gCurrentBGM = -1;
    gNowPlaying[0] = '\0';
}

void audioPlayArena(int arenaId) {
    if (!gAudioReady) return;

    if (arenaId == 0 || arenaId == 1) {
        if (gCurrentBGM == 0) return;
        closeCurrentBGM();
        if (openAndPlay(gBahlilAbs, "bgm", 0, 0, 1)) {
            gCurrentBGM = 0;
            sprintf(gNowPlaying, "Now Playing: Mas Bahlil Ganteng");
            printf("[Audio] Playing: Bahlil BGM (Arena %d)\n", arenaId);
        }
    }
    else if (arenaId == 2) {
        if (gCurrentBGM == 2) return;
        closeCurrentBGM();
        if (openAndPlay(gPrabowoAbs, "bgm", 30000, 62000, 0)) {
            gCurrentBGM = 2;
            sprintf(gNowPlaying, "Now Playing: Oke Gas Prabowo Gibran");
            printf("[Audio] Playing: Prabowo BGM (Boss Room)\n");
        }
    }
}

void audioStop(void) {
    if (!gAudioReady) return;
    closeCurrentBGM();
    gCurrentBGM = -1;
    gNowPlaying[0] = '\0';
}

void audioCleanup(void) {
    audioStop();
    gAudioReady = 0;
}

const char* audioGetNowPlaying(void) {
    return gNowPlaying;
}
