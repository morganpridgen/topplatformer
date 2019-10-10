#include "ambience.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "helpers.h"
#define aLen 76
#define aCount 6

Mix_Chunk *ambience = nullptr;
int aChannel = -1;
unsigned int time;
unsigned char currentA = 0;

void loadAmbience() {
  ambience = Mix_LoadWAV(pathForData("sfx/ambient.wav"));
  Mix_VolumeChunk(ambience, MIX_MAX_VOLUME / 2);
  aChannel = Mix_PlayChannel(-1, ambience, -1);
  if (aChannel == -1) {
    freeAmbience();
    return;
  }
  Mix_Volume(aChannel, 0);
}

void freeAmbience() {
  Mix_ExpireChannel(aChannel, 0);
  Mix_FreeChunk(ambience);
}

void runAmbience() {
  if (ambience && (aChannel == -1 || !Mix_Playing(aChannel))) {
    aChannel = Mix_PlayChannel(-1, ambience, -1);
    Mix_Volume(aChannel, 0);
  }
  if (time == 0) currentA = rng() % aCount;
  if (time == currentA * aLen) Mix_Volume(aChannel, MIX_MAX_VOLUME);
  if (time == (currentA + 1) * aLen) Mix_Volume(aChannel, 0);
  time++;
  if (time == aCount * aLen) time = 0;
}

void resetAmbience() {
  Mix_Volume(aChannel, 0);
  time = 0;
}