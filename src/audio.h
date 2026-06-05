#ifndef AUDIO_H
#define AUDIO_H

void audioInit(void);

/* Sound Effects */
void sfxShoot(int weaponType);   /* 0=pistol, 1=shotgun, 2=m416 */
void sfxReload(void);
void sfxFootstep(void);
void sfxEnemyDeath(void);

#endif
