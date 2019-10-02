#include "enemy.h"

#include <cmath>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "window.h"
#include "helpers.h"
#include "terrain.h"
#include "player.h"
#ifdef _WIN32
  //#include <string.h>
  #define abs(x) (x<0?-x:x)
#endif

SDL_Texture *triImg = nullptr;
SDL_Texture *badFishImg = nullptr;
SDL_Texture *badFishProjImg = nullptr;
SDL_Texture *sharkImg = nullptr;
Mix_Chunk *splash = nullptr;
Mix_Chunk *laser = nullptr;

Enemy::Enemy() {
  info = {0, 0, 0, 0, 128, 0};
  active = 1;
}

void Enemy::update() {
  if (info.moveTimer == 0) {
    int usedDirs = 0;
    int tileType;
    Tile *tile;
    checkDirs:
    tickRand();
    int d = info.seed % 4;
    info.xVel = 0;
    info.zVel = 0;
    if (nP(usedDirs == 0b1111)) goto ignoreDirs;
    if (d == 0) info.xVel = 1;
    if (d == 1) info.zVel = 1;
    if (d == 2) info.xVel = -1;
    if (d == 3) info.zVel = -1;
    usedDirs |= 1 << d;
    tile = terrainAt(info.x + info.xVel * 256, info.z + info.zVel * 256);
    tileType = tile->type;
    delete tile;
    if (isSolidType(tileType) || tileType == 2) goto checkDirs;
    info.xVel *= 16;
    info.zVel *= 16;
    ignoreDirs:
    info.moveTimer = 64 + 256 / 16;
  } else {
    if (info.moveTimer < 65) {
      info.xVel = 0;
      info.zVel = 0;
    }
    info.x += info.xVel;
    info.z += info.zVel;
    info.moveTimer--;
  }
}

void Enemy::render(Window &win, Camera cam) {
  SDL_Rect rect = {(WORLDTOPIX(info.x, cam.x) - 7) * win.info.wRatio, (WORLDTOPIX(info.z, cam.y) - 7) * win.info.hRatio, 16 * win.info.wRatio, 16 * win.info.hRatio};
  SDL_RenderCopy(win.getRender(), triImg, NULL, &rect);
}

void Enemy::tickRand() {
  if (nP(info.seed == 0)) info.seed = 1;
  bool newBit = ((info.seed >> 2) & 1) ^ ((info.seed >> 3) & 1);
  info.seed <<= 1;
  info.seed |= newBit;
}

void BadFish::update(Player &ply, Particle *particles[particleCount]) {
  info.playerClose = abs(info.x - ply.getInfo().x) < 1024 && abs(info.z - ply.getInfo().z) < 1024;
  if (info.playerClose && shot.x == shot.targetX && shot.z == shot.targetZ) info.shotTimer--;
  if (info.shotTimer == 0) {
    shot = {info.x, info.z, ply.getInfo().x, ply.getInfo().z};
    info.shotTimer = 64;
    for (int i = 0; i < 16; i++) {
      float targetAngle = atan2(ply.getInfo().z - info.z, ply.getInfo().x - info.x);
      addParticle(particles, {info.x, 0, info.z, cos(targetAngle) * float(8 * (i + 1)), 0, sin(targetAngle) * float(8 * (i + 1)), i % 4}, 1);
    }
    Mix_PlayChannel(-1, laser, 0);
  }
  shot.x += (shot.targetX - shot.x) / 8;
  shot.z += (shot.targetZ - shot.z) / 8;
  if (shot.x != shot.targetX || shot.z != shot.targetZ) {
    shot.targetX += (ply.getInfo().x - shot.targetX) / (abs(info.x - shot.x) / 2 + 1);
    shot.targetZ += (ply.getInfo().z - shot.targetZ) / (abs(info.z - shot.z) / 2 + 1);
  }
  /*if (shot.xVel > 0) shot.xVel -= 2;
  if (shot.xVel < 0) shot.xVel += 2;
  if (shot.zVel > 0) shot.zVel -= 2;
  if (shot.zVel < 0) shot.zVel += 2;*/
  if (abs(shot.targetX - shot.x) < 16) shot.x = shot.targetX;
  if (abs(shot.targetZ - shot.z) < 16) shot.z = shot.targetZ;
}

void BadFish::render(Window &win, Camera cam, bool simpleEffects) {
  SDL_Rect rect = {(WORLDTOPIX(info.x, cam.x) - 7) * win.info.wRatio, (WORLDTOPIX(info.z, cam.y) - 7) * win.info.hRatio, 16 * win.info.wRatio, 16 * win.info.hRatio};
  /*setColorFromPalette(win, 0);
  SDL_RenderFillRect(win.getRender(), &rect);*/
  SDL_Rect clip = {0, 0, 16, 16};
  if (info.playerClose) {
    if (shot.x != shot.targetX || shot.z != shot.targetZ) clip.x = 16;
    else clip.x = 32;
  }
  SDL_RenderCopy(win.getRender(), badFishImg, &clip, &rect);
  if (shot.x != shot.targetX || shot.z != shot.targetZ) {
    rect = {(WORLDTOPIX(shot.x, cam.x) - 3) * win.info.wRatio, (WORLDTOPIX(shot.z, cam.y) - 3) * win.info.hRatio, 8 * win.info.wRatio, 8 * win.info.hRatio};
    /*setColorFromPalette(win, 1);
    SDL_RenderFillRect(win.getRender(), &rect);*/
    clip = {((getTimer() / 2) % 4) * 8, 0, 8, 8};
    SDL_RenderCopy(win.getRender(), badFishProjImg, &clip, &rect);
    if ((getTimer() / 2) % 2 || simpleEffects) {
      clip = {((getTimer() / 4) % 4) * 8, 8, 8, 8};
      SDL_RenderCopy(win.getRender(), badFishProjImg, &clip, &rect);
    }
  }
}

void Shark::update(Player &ply, Particle *particles[particleCount]) {
  if (info.hiding) {
    if (getTimer() % 8 == 0) addParticle(particles, {info.x, -1, info.z, (rng() % 65) - 32, -16, (rng() % 65) - 32, 0});
    if (abs(info.x - ply.getInfo().x) < 1024 && abs(info.z - ply.getInfo().z) < 1024) {
      info.yVel = -48;
      info.firstHop = 1;
      info.hiding = 0;
      float targetAngle = atan2(ply.getInfo().z - info.z, ply.getInfo().x - info.x);
      info.xVel = cos(targetAngle) * 16;
      info.zVel = sin(targetAngle) * 16;
      for (int k = 0; k < 16; k++) {
        addParticle(particles, {info.x, -1, info.z, cos((k * 22.5) / (180 / 3.14)) * 64, -16, sin((k * 22.5) / (180 / 3.14)) * 64, 2});
      }
      Mix_PlayChannel(-1, splash, 0);
    }
  } else {
    info.x += info.xVel;
    info.y += info.yVel;
    info.z += info.zVel;
    Tile *tile = terrainAt(info.x, info.z);
    if (!isAirType(tile->type) && (getSwitch() ? tile->type != 12 : tile->type != 13)) {
      /*info.xVel = 0 - info.xVel;
      info.zVel = 0 - info.zVel;
      info.x += info.xVel;
      info.z += info.zVel;*/
      delete tile;
      info.x -= info.xVel;
      tile = terrainAt(info.x, info.z);;
      if (!isAirType(tile->type) && (getSwitch() ? tile->type != 12 : tile->type != 13)) {
        info.x += info.xVel;
        info.z -= info.zVel;
        info.zVel = 0 - info.zVel;
      } else info.xVel = 0 - info.xVel;
    }
    info.yVel += 2;
    if (info.y > 0) {
      //info.onGround = 1;
      if (tile->type == 2) info.hiding = 1;
      info.y = 0;
      if (info.firstHop || abs(info.x - ply.getInfo().x) < 1024 && abs(info.z - ply.getInfo().z) < 512) {
        float targetAngle = atan2(ply.getInfo().z - info.z, ply.getInfo().x - info.x);
        info.xVel = cos(targetAngle) * 8;
        info.zVel = sin(targetAngle) * 8;
      }
      info.yVel = -16;
      info.firstHop = 0;
    }
    if (tile) delete tile;
  }
}

void Shark::render(Window& win, Camera cam, bool simpleEffects) {
  if (!info.hiding) {
    SDL_Rect rect;
    setColorFromPalette(win, 1);
    if (simpleEffects) {
      rect = {(WORLDTOPIX(info.x, cam.x) - 5) * win.info.wRatio, (WORLDTOPIX(info.z, cam.y) - 5) * win.info.hRatio, 12 * win.info.wRatio, 12 * win.info.hRatio};
      SDL_RenderFillRect(win.getRender(), &rect);
    } else {
      for (int i = 0; i < 12 * win.info.wRatio; i++) {
        for (int j = 0; j < 12 * win.info.hRatio; j++) {
          if (int((WORLDTOPIX(info.x, cam.x) - 5) * win.info.wRatio + i + (WORLDTOPIX(info.z, cam.y) - 5) * win.info.hRatio + j + getTimer()) % 2 == 0) SDL_RenderDrawPoint(win.getRender(), (WORLDTOPIX(info.x, cam.x) - 5) * win.info.wRatio + i, (WORLDTOPIX(info.z, cam.y) - 5) * win.info.hRatio + j);
        }
      }
    }
    rect = {(WORLDTOPIX(info.x, cam.x) - 7) * win.info.wRatio, (WORLDTOPIX(info.z + info.y, cam.y) - 7) * win.info.hRatio, 16 * win.info.wRatio, 16 * win.info.hRatio};
    //SDL_RenderCopy(win.getRender(), sharkImg, NULL, &rect);
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    if (info.xVel > 0) flip = SDL_FLIP_HORIZONTAL;
    SDL_RenderCopyEx(win.getRender(), sharkImg, NULL, &rect, 0, NULL, flip);
  }
}

void enemyLoadRes(Window& win) {
  triImg = loadTexture(pathForData("imgs/enemy.png"), win);
  badFishImg = loadTexture(pathForData("imgs/badfish.png"), win);
  badFishProjImg = loadTexture(pathForData("imgs/badfishshot.png"), win);
  sharkImg = loadTexture(pathForData("imgs/shark.png"), win);
  splash = Mix_LoadWAV(pathForData("sfx/splash.wav"));
  laser = Mix_LoadWAV(pathForData("sfx/shot.wav"));
}

void enemyFreeRes() {
  SDL_DestroyTexture(triImg);
  SDL_DestroyTexture(badFishImg);
  SDL_DestroyTexture(badFishProjImg);
  SDL_DestroyTexture(sharkImg);
  if (splash) Mix_FreeChunk(splash);
  if (laser) Mix_FreeChunk(laser);
}