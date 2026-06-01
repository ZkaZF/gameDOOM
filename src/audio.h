#ifndef AUDIO_H
#define AUDIO_H

void audioInit(void);
void audioPlayArena(int arenaId);   /* 0=atas, 1=bawah, 2=boss */
void audioStop(void);
void audioCleanup(void);
const char* audioGetNowPlaying(void);

#endif
