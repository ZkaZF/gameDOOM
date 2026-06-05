
#include "audio.h"
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>

#pragma comment(lib, "winmm.lib")

static char gSfxShoot[3][MAX_PATH];
static char gSfxReload[MAX_PATH];
static char gSfxFootstep[MAX_PATH];
static char gSfxEnemyDeath[MAX_PATH];

/* Menginisialisasi path untuk file audio SFX yang digunakan dalam game */
void audioInit(void) {
    GetFullPathNameA("assets/sfx_pistol.wav",     MAX_PATH, gSfxShoot[0],    NULL);
    GetFullPathNameA("assets/sfx_shotgun.wav",    MAX_PATH, gSfxShoot[1],    NULL);
    GetFullPathNameA("assets/sfx_m416.wav",       MAX_PATH, gSfxShoot[2],    NULL);
    GetFullPathNameA("assets/sfx_reload.wav",     MAX_PATH, gSfxReload,      NULL);
    GetFullPathNameA("assets/sfx_footstep.wav",   MAX_PATH, gSfxFootstep,    NULL);
    GetFullPathNameA("assets/sfx_enemy_death.wav",MAX_PATH, gSfxEnemyDeath,  NULL);
}

/* Memutar sound effect tembakan berdasarkan jenis senjata (0=Pistol, 1=Shotgun, 2=M416) */
void sfxShoot(int weaponType) {
    if (weaponType >= 0 && weaponType < 3)
        PlaySoundA(gSfxShoot[weaponType], NULL, SND_ASYNC | SND_FILENAME | SND_NOSTOP);
}

/* Memutar sound effect saat senjata sedang di-reload */
void sfxReload(void) {
    PlaySoundA(gSfxReload, NULL, SND_ASYNC | SND_FILENAME);
}

/* Memutar sound effect langkah kaki pemain saat bergerak */
void sfxFootstep(void) {
    PlaySoundA(gSfxFootstep, NULL, SND_ASYNC | SND_FILENAME | SND_NOSTOP);
}

/* Memutar sound effect geraman/teriakan saat musuh mati */
void sfxEnemyDeath(void) {
    PlaySoundA(gSfxEnemyDeath, NULL, SND_ASYNC | SND_FILENAME | SND_NOSTOP);
}
