#include "player.h"
#include <algorithm>
#include <cmath>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "window.h"
#include "helpers.h"
#include "controller.h"
#include "terrain.h"
#include "enemy.h"
#define motionAccuracy 4

bool infSpeed = 0;
bool hRumble = 1;
bool assistWater = 0;
bool assistJump = 0;

Player::Player() {
  info.x = 0;
  info.y = -8256;
  info.z = 0;
  info.xVel = 0;
  info.yVel = 0;
  info.zVel = 0;
  info.onGround = 1;
  info.flyAnim = 1;
  info.airJump = 0;
  info.airDash = 0;
  info.lastJump = 0;
  box.changeBox(info.x, info.z, 64, 64);
  img = nullptr;
  jump = nullptr;
  bump = nullptr;
  splash = nullptr;
  screenFinish = nullptr;
  jump2 = nullptr;
  dash = nullptr;
}

void Player::end() {
  if (img) SDL_DestroyTexture(img);
  if (face) SDL_DestroyTexture(face);
  if (jump) Mix_FreeChunk(jump);
  if (bump) Mix_FreeChunk(bump);
  if (splash) Mix_FreeChunk(splash);
  if (screenFinish) Mix_FreeChunk(screenFinish);
  if (jump2) Mix_FreeChunk(jump2);
  if (dash) Mix_FreeChunk(dash);
  if (burn) Mix_FreeChunk(burn);
}

bool Player::init(Window& win, bool heavyRumble) {
  img = loadTexture(pathForData("imgs/player/player.png"), win);
  if (!img) return 0;
  face = loadTexture(pathForData("imgs/player/face.png"), win);
  if (!face) return 0;
  jump = Mix_LoadWAV(pathForData("sfx/jump.wav"));
  if (!jump) return 0;
  bump = Mix_LoadWAV(pathForData("sfx/bump.wav"));
  if (!bump) return 0;
  splash = Mix_LoadWAV(pathForData("sfx/splash.wav"));
  if (!splash) return 0;
  screenFinish = Mix_LoadWAV(pathForData("sfx/screenfinish.wav"));
  if (!screenFinish) return 0;
  jump2 = Mix_LoadWAV(pathForData("sfx/jump2.wav"));
  if (!jump2) return 0;
  dash = Mix_LoadWAV(pathForData("sfx/dash.wav"));
  if (!dash) return 0;
  burn = Mix_LoadWAV(pathForData("sfx/burn.wav"));
  if (!burn) return 0;
  hRumble = heavyRumble;
  return 1;
}

int Player::update(Controller* ctrl, bool assist, Particle *particles[particleCount], Enemy *enemies[enemyCount], BadFish *badFish[badFishCount], Shark *sharks[sharkCount]) {
  static bool lastSwitch = 0;
  static int stuckFrames = 0;
  int code = 0;
  if (yP(!info.flyAnim)) {
    if (info.xVel > (ctrl->aX / (2 - infSpeed)) / (!(ctrl->b & CTRL_ACTION_GAME_RUN) + 1)) info.xVel -= info.flying ? 1 : 4;
    if (info.xVel < (ctrl->aX / (2 - infSpeed)) / (!(ctrl->b & CTRL_ACTION_GAME_RUN) + 1)) info.xVel += info.flying ? 1 : 4;
    if (info.zVel > (ctrl->aY / (2 - infSpeed)) / (!(ctrl->b & CTRL_ACTION_GAME_RUN) + 1)) info.zVel -= info.flying ? 1 : 4;
    if (info.zVel < (ctrl->aY / (2 - infSpeed)) / (!(ctrl->b & CTRL_ACTION_GAME_RUN) + 1)) info.zVel += info.flying ? 1 : 4;
    if (!info.flying && abs(info.xVel) < 4) info.xVel = 0;
    if (!info.flying && abs(info.zVel) < 4) info.zVel = 0;
    bool sliding = (abs((ctrl->aX / 2 / (!(ctrl->b & CTRL_ACTION_GAME_RUN) + 1)) - info.xVel) > 8) || (abs((ctrl->aY / 2 / (!(ctrl->b & CTRL_ACTION_GAME_RUN) + 1)) - info.zVel) > 8);
    if (sliding && info.onGround) {
      if (hRumble) ctrlRumble(*ctrl, (gamemax(abs(info.xVel), abs(info.zVel)) / 64.0f) * 1.5, 16);
      for (int i = 0; i < 8; i++) {
        if (rng() % 4 == 0) continue;
        addParticle(particles, {info.x, -1, info.z, (info.xVel * -0.5 * ((float(rng() - 32768) / 32768) / 2)) + 16 * cos((i * 45) / (180 / 3.14)), -16, (info.zVel * -0.5 * ((float(rng() - 32768) / 32768) / 2)) + 16 * sin((i * 45) / (180 / 3.14)), 2});
      }
    }
    if (!infSpeed) {
      info.xVel = gamemax(gamemin(info.xVel, -64 * (1 + info.airDash)), 64 * (1 + info.airDash));
      info.zVel = gamemax(gamemin(info.zVel, -64 * (1 + info.airDash)), 64 * (1 + info.airDash));
    }
    bool hasChangedX = 0;
    bool hasChangedZ = 0;
    bool hasBumped = 0;
    if (info.onGround) infSpeed = 0;
    for (int i = 0; i < motionAccuracy; i++) {
      info.x += info.xVel / motionAccuracy;
      info.y += info.yVel / motionAccuracy;
      info.z += info.zVel / motionAccuracy;
      box.changeBox(info.x, info.z, 64, 64);
      for (char j = -1; j <= 1; j++) {
        for (char k = -1; k <= 1; k++) {
          Tile *tileInfo = terrainAt(info.x + (j * 256), info.z + (k * 256));
          if (box.collide(tileInfo->all)) {
            switch (tileInfo->type) {
              case 0:
              case 6:
              case 11:
              case 12:
              case 13:
              case 15:
              case 17:
              case 19:
                if (getSwitch() ? tileInfo->type == 12 : tileInfo->type == 13) break;
                if (!lastSwitch && tileInfo->type == 11) {
                  lastSwitch = 1;
                  setSwitch(!getSwitch());
                }
                if (!lastSwitch && tileInfo->type == 15) killSClock();
                if (!(hasChangedX && hasChangedZ) && !hasBumped) {
                  ctrlRumble(*ctrl, (float(gamemin(abs(info.xVel), abs(info.zVel))) / 64.0f) * (float(hRumble) / 2 + 0.5), 100);
                  Mix_PlayChannel(-1, bump, 0);
                  hasBumped = 1;
                }
                if (i % 2) {
                  info.xVel *= -1;
                  info.x += info.xVel;
                  if (hasChangedZ) {
                    info.zVel *= -1;
                    info.z += 2 * info.zVel;
                  }
                  hasChangedX = !hasChangedX;
                } else {
                  info.zVel *= -1;
                  info.z += info.zVel;
                  if (hasChangedX) {
                    info.xVel *= -1;
                    info.x += 2 * info.xVel;
                  }
                  hasChangedZ = !hasChangedZ;
                }
                box.changeBox(info.x, info.z, 64, 64);
                break;
              case 1:
                assistWater = assist;
                lastSwitch = 0;
                break;
              case 2:
                if (info.onGround) {
                  if (assistWater && tileInfo->type == 2) {
                    info.x -= info.xVel / motionAccuracy;
                    info.z -= info.zVel / motionAccuracy;
                    if (i + 1 == motionAccuracy) {
                      info.x -= info.xVel / motionAccuracy;
                      info.z -= info.zVel / motionAccuracy;
                      info.xVel = 0;
                      info.zVel = 0;
                    }
                    break;
                  }
                  Mix_PlayChannel(-1, splash, 0);
                  info.yVel = 0;
                  info.onGround = 0;
                  info.faceAnim = 2;
                  code = 2;
                  for (int l = 0; l < 16; l++) {
                    addParticle(particles, {info.x, -1, info.z, cos((l * 22.5) / (180 / 3.14)) * 64, -16, sin((l * 22.5) / (180 / 3.14)) * 64, 3});
                  }
                }
                break;
              case 3:
                if (info.onGround || assist) {
                  Mix_PlayChannel(-1, screenFinish, 0);
                  i = motionAccuracy;
                  code = 1;
                  info.faceAnim = 1;
                  for (int l = 0; l < 16; l++) {
                    addParticle(particles, {info.x, -1, info.z, cos((l * 22.5) / (180 / 3.14)) * 64.0, -16, sin((l * 22.5) / (180 / 3.14)) * 64.0, l % 4});
                  }
                }
                break;
              /*case 4:
                if (!info.onGround && getTimer() % 2 && info.yVel > 0) {
                  short targetXVel = (((round(info.x / 256.0 + i) * 256)) - info.x) * -1;
                  short targetZVel = (((round(info.z / 256.0 + j) * 256)) - info.z);
                  if (info.xVel < targetXVel) info.xVel += 8;
                  if (info.xVel > targetXVel) info.xVel -= 8;
                  if (info.zVel < targetZVel) info.zVel += 8;
                  if (info.zVel > targetZVel) info.zVel -= 8;
                }
                break;*/
              case 5:
                if (info.onGround) infSpeed = 1;
                break;
              case 7:
                if (info.onGround) {
                  if (assistWater && tileInfo->type == 2) {
                    info.x -= info.xVel / motionAccuracy;
                    info.z -= info.zVel / motionAccuracy;
                    if (i + 1 == motionAccuracy) {
                      info.x -= info.xVel / motionAccuracy;
                      info.z -= info.zVel / motionAccuracy;
                      info.xVel = 0;
                      info.zVel = 0;
                    }
                    break;
                  }
                  Mix_PlayChannel(-1, burn, 0);
                  info.yVel = 0;
                  info.onGround = 0;
                  info.faceAnim = 3;
                  code = 2;
                  for (int l = 0; l < 16; l++) {
                    addParticle(particles, {info.x, -1, info.z, cos((l * 22.5) / (180 / 3.14)) * 64, -16, sin((l * 22.5) / (180 / 3.14)) * 64, 3});
                  }
                }
                break;
              case 18:
                if ((ctrl->aX * ctrl->aX) + (ctrl->aY * ctrl->aY) > 32 * 32) {
                  info.xVel = (ctrl->aX / 4) * 3;
                  info.zVel = (ctrl->aY / 4) * 3;
                }
                break;
            }
          }
          /*if ((tileInfo->type == 4 || tileInfo->type == 3) && !info.onGround && getTimer() % 2 && info.yVel > 0) {
            short targetXVel = ((((round(float(info.x) / 256.0) + float(i)) * 256)) - info.x);
            short targetZVel = ((((round(float(info.z) / 256.0) + float(j)) * 256)) - info.z);
            if (info.xVel < targetXVel) info.xVel += 4 * (1 + (tileInfo->type == 4));
            if (info.xVel > targetXVel) info.xVel -= 4 * (1 + (tileInfo->type == 4));
            if (info.zVel < targetZVel) info.zVel += 4 * (1 + (tileInfo->type == 4));
            if (info.zVel > targetZVel) info.zVel -= 4 * (1 + (tileInfo->type == 4));
          }*/
          delete tileInfo;
        }
      }
      for (int j = 0; j < enemyCount; j++) {
        if (enemies[j]) {
          if (info.onGround && box.collide(enemies[j]->getHitbox())) {
            Mix_PlayChannel(-1, splash, 0);
            info.yVel = 0;
            info.onGround = 0;
            info.faceAnim = 2;
            code = 2;
            for (int k = 0; k < 16; k++) {
              addParticle(particles, {info.x, -1, info.z, cos((k * 22.5) / (180 / 3.14)) * 64, -16, sin((k * 22.5) / (180 / 3.14)) * 64, 3});
            }
          }
        }
      }
      for (int j = 0; j < badFishCount; j++) {
        if (badFish[j]) {
          if (info.onGround && box.collide(badFish[j]->getHitbox())) {
            Mix_PlayChannel(-1, splash, 0);
            info.yVel = 0;
            info.onGround = 0;
            info.faceAnim = 3;
            code = 2;
            for (int k = 0; k < 16; k++) {
              addParticle(particles, {info.x, -1, info.z, cos((k * 22.5) / (180 / 3.14)) * 64, -16, sin((k * 22.5) / (180 / 3.14)) * 64, 3});
            }
          }
        }
      }
      for (int j = 0; j < sharkCount; j++) {
        if (sharks[j]) {
          SharkInfo sInfo = sharks[j]->getInfo();
          if (abs(info.x - sInfo.x) < 96 && abs(info.y - sInfo.y) < 96 && abs(info.z - sInfo.z) < 96) {
            Mix_PlayChannel(-1, splash, 0);
            info.yVel = 0;
            info.onGround = 0;
            info.faceAnim = 2;
            code = 2;
          }
        }
      }
    }
    Tile *tileInfo = terrainAt(info.x, info.z);
    info.flying = tileInfo->type == 16;
    if (nP(tileInfo->type == 0 || tileInfo->type == 6 || tileInfo->type == 11 || (getSwitch() ? tileInfo->type == 13 : tileInfo->type == 12) || tileInfo->type == 15 || tileInfo->type == 17 || tileInfo->type == 19)) {
      stuckFrames++;
      delete tileInfo;
      tileInfo = terrainAt(info.x + stuckFrames * 4, info.z);
      if (!(tileInfo->type == 0 || tileInfo->type == 6 || tileInfo->type == 11 || (getSwitch() ? tileInfo->type == 13 : tileInfo->type == 12) || tileInfo->type == 15 || tileInfo->type == 17 || tileInfo->type == 19)) info.x += stuckFrames * 4;
      else {
        delete tileInfo;
        tileInfo = terrainAt(info.x - stuckFrames * 4, info.z);
        if (!(tileInfo->type == 0 || tileInfo->type == 6 || tileInfo->type == 11 || (getSwitch() ? tileInfo->type == 13 : tileInfo->type == 12) || tileInfo->type == 15 || tileInfo->type == 17 || tileInfo->type == 19)) info.x -= stuckFrames * 4;
        else {
          delete tileInfo;
          tileInfo = terrainAt(info.x, info.z + stuckFrames * 4);
          if (!(tileInfo->type == 0 || tileInfo->type == 6 || tileInfo->type == 11 || (getSwitch() ? tileInfo->type == 13 : tileInfo->type == 12) || tileInfo->type == 15 || tileInfo->type == 17 || tileInfo->type == 19)) info.z += stuckFrames * 4;
          else {
            delete tileInfo;
            tileInfo = terrainAt(info.x, info.z - stuckFrames * 4);
            if (!(tileInfo->type == 0 || tileInfo->type == 6 || tileInfo->type == 11 || (getSwitch() ? tileInfo->type == 13 : tileInfo->type == 12) || tileInfo->type == 15 || tileInfo->type == 17 || tileInfo->type == 19)) info.z -= stuckFrames * 4;
          }
        }
      }
      if (stuckFrames > 16) {
        info.yVel = 0;
        info.onGround = 0;
        info.faceAnim = 2;
        code = 2;
      }
    } else stuckFrames = 0;
    delete tileInfo;
    if (nP(hasBumped)) {
      for (int i = 0; i < 7; i++) {
        addParticle(particles, {info.x, info.y, info.z, (info.xVel + ((rng() % 33) - 16)) * 1.5, info.yVel, (info.zVel + ((rng() % 33) - 16)) * 1.5, 2});
      }
    }
    if (info.flying) {
      info.onGround = 0;
      info.airBoost = 1;
      short targetYV = (int((sin(float(getTimer()) / 16.0) * -128.0) - 256.0) - info.y) / 8;
      if (info.yVel < targetYV) info.yVel++;
      if (info.yVel > targetYV) info.yVel--;
      info.y += info.yVel;
    } else {
      if (info.airBoost) {
        for (int i = 0; i < 4; i++) addParticle(particles, {info.x, info.y, info.z, cos(float(getTimer()) / 8.0 + (i * 3.14 / 2)) * 32.0, 16, sin(float(getTimer()) / 8.0 + (i * 3.14 / 2)) * 32.0, 3});
      }
      if (!info.onGround && (ctrl->b & CTRL_ACTION_GAME_JUMP) && !(info.airJump && (assistJump || !assist)) && !info.lastJump) {
        assistJump = info.airJump;
        info.airJump = 1;
        info.yVel = -32;
        Mix_PlayChannel(-1, jump2, 0);
      }
      if ((ctrl->b & CTRL_ACTION_GAME_JUMP) && info.onGround && !info.lastJump) {
        info.yVel = -32;
        info.onGround = 0;
        Mix_PlayChannel(-1, jump, 0);
      }
      if (!info.onGround) {
        assistWater = 0;
        if (!info.airBoost || getTimer() % 2) {
          info.yVel += 2;
          if (!info.airBoost && !(ctrl->b & CTRL_ACTION_GAME_JUMP)) info.yVel += 2;
        }
      }
      if (!info.onGround && (ctrl->b & CTRL_ACTION_GAME_SQUISH)) {
        info.xVel = 0;
        info.yVel = 64;
        info.zVel = 0;
      }
      /*if (!info.onGround && (ctrl->b & 0b10000000)) {
        if (!info.airDash) {
          info.xVel = (ctrl->aX / 1);
          info.yVel = 0;
          info.zVel = (ctrl->aY / 1);
          Mix_PlayChannel(-1, dash, 0);
        }
        info.airDash = 1;
        //if (getTimer() % 4 == 0) info.yVel = 0;
      }*/
    }
    if (nP(info.x < -(worldSizeX - 34))) {
      info.x = -(worldSizeX - 34);
      info.xVel *= -1;
      ctrlRumble(*ctrl, (float(abs(info.xVel)) / 64.0f) * 1.0f, 100);
    }
    if (nP(info.x > worldSizeX - 34)) {
      info.x = worldSizeX - 34;
      info.xVel *= -1;
      ctrlRumble(*ctrl, (float(abs(info.xVel)) / 64.0f) * 1.0f, 100);
    }
    if (nP(info.z < -(worldSizeZ - 34))) {
      info.z = -(worldSizeZ - 34);
      info.zVel *= -1;
      ctrlRumble(*ctrl, (float(abs(info.zVel)) / 64.0f) * 1.0f, 100);
    }
    if (nP(info.z > worldSizeZ - 34)) {
      info.z = worldSizeZ - 34;
      info.zVel *= -1;
      ctrlRumble(*ctrl, (float(abs(info.zVel)) / 64.0f) * 1.0f, 100);
    }
    if (info.y > 0) {
      info.y = 0;
      info.yVel = 0;
      info.onGround = 1;
      info.airJump = 0;
      assistJump = 0;
      info.airDash = 0;
      info.airBoost = 0;
      ctrlRumble(*ctrl, 0.9, 100);
      for (int i = 0; i < 8; i++) {
        addParticle(particles, {info.x, -1, info.z, cos((i * 45) / (180 / 3.14)) * 32.0, -16, sin((i * 45) / (180 / 3.14)) * 32.0, 2});
      }
    }
    if (nP(code)) {
      info.flyAnim = 1;
      info.onGround = 0;
      if (info.y < 1) info.y = 1;
    }
  } else {
    if (info.xVel > 0) info.xVel -= 2;
    if (info.xVel < 0) info.xVel += 2;
    if (info.zVel > 0) info.zVel -= 2;
    if (info.zVel < 0) info.zVel += 2;
    if (info.onGround) {
      if (info.yVel < 0) info.yVel = 0;
      info.yVel += 2;
      info.y += info.yVel;
      if (info.y > 0) {
        info.flyAnim = 0;
        info.y = 0;
      }
    } else {
      if (hRumble || (sin(getTimer()) > 0.7 && info.yVel > -32)) ctrlRumble(*ctrl, 0.7f + (sin(getTimer()) / 5.0f), 16);
      if (info.yVel > 0) info.yVel = 0;
      info.yVel -= 1;
      info.y += info.yVel;
      if (info.y < -8192) code = 3;
    }
  }
  info.lastJump = (ctrl->b & CTRL_ACTION_GAME_JUMP);
  return code;
}

void Player::render(Window &win, Camera cam, Controller *ctrl, bool simpleEffects, char layer) {
  SDL_Rect rect;
  if (layer == 0 && ((!info.onGround) || info.flyAnim)) {
    Tile *tileInfo = terrainAt(info.x, info.z);
    bool shadowDown = tileInfo->type == 2 || tileInfo->type == 16;
    delete tileInfo;
    setColorFromPalette(win, 1);
    if (nP(simpleEffects)) {
      rect = {(WORLDTOPIX(info.x, cam.x) - 3) * win.info.wRatio, (WORLDTOPIX(info.z, cam.y) - 3 + (8 * shadowDown)) * win.info.hRatio, 8 * win.info.wRatio, 8 * win.info.hRatio};
      SDL_RenderFillRect(win.getRender(), &rect);
    } else {
      for (int i = 0; i < 8 * win.info.wRatio; i++) {
        for (int j = 0; j < 8 * win.info.hRatio; j++) {
          if (int((WORLDTOPIX(info.x, cam.x) - 3) * win.info.wRatio + i + (WORLDTOPIX(info.z, cam.y) - 3 + (8 * shadowDown)) * win.info.hRatio + j + getTimer()) % 2 == 0) SDL_RenderDrawPoint(win.getRender(), (WORLDTOPIX(info.x, cam.x) - 3) * win.info.wRatio + i, (WORLDTOPIX(info.z, cam.y) - 3 + (8 * shadowDown)) * win.info.hRatio + j);
        }
      }
    }
  }
  if (yP((info.onGround ? layer == 0 : layer == 1) && ((getTimer() % 2) || !info.flyAnim))) {
    rect = {(WORLDTOPIX(info.x, cam.x) - 3) * win.info.wRatio, (WORLDTOPIX(info.z + info.y, cam.y) - 3) * win.info.hRatio, 8 * win.info.wRatio, 8 * win.info.hRatio};
    /*SDL_SetRenderDrawColor(win.getRender(), 0x20, 0xF0, 0x00, 0xFF);
    SDL_RenderFillRect(win.getRender(), &rect);*/
    SDL_RenderCopy(win.getRender(), img, NULL, &rect);
    rect = {(WORLDTOPIX(info.x + ((info.xVel ? info.xVel * (2 - infSpeed) : ctrl->a2X) / 16), cam.x) - 3) * win.info.wRatio, (WORLDTOPIX(info.z + info.y + ((info.zVel ? info.zVel * (2 - infSpeed) : ctrl->a2Y) / 16), cam.y) - 3) * win.info.hRatio, 8 * win.info.wRatio, 8 * win.info.hRatio};
    SDL_Rect clip = {8 * info.faceAnim, 0, 8, 8};
    SDL_RenderCopy(win.getRender(), face, &clip, &rect);
  }
}

void Player::updateCamera(Camera &cam, Controller *ctrl, bool instant) {
  int cTargX = gamemax(gamemin(info.x + (64 * (info.xVel / 4)) - 2560, (-worldSizeX * 2) + 5120 * 2), (worldSizeX * 2) - 5120 * 3) + (ctrl->a2X * 8);
  int cTargY = gamemax(gamemin(info.z + (64 * (info.zVel / 4)) - 1440, (-worldSizeZ * 2) + 2880 * 2), (worldSizeZ * 2) - 2880 * 3)  + (ctrl->a2Y * 8);
  /*if (cTargX - cam.x < 1280) cam.x += gamemin(cTargX - cam.x - 1280, -56);
  if (cTargX - cam.x > 3840) cam.x += gamemax(cTargX - cam.x - 3840, 56);
  if (cTargY - cam.y < 720) cam.y += gamemin(cTargY - cam.y - 720, -56);
  if (cTargY - cam.y > 2160) cam.y += gamemax(cTargY - cam.y - 2160, 56);
  while (cTargX - cam.x < 640) cam.x -= 1;
  while (cTargX - cam.x > 4480) cam.x += 1;
  while (cTargY - cam.y < 360) cam.y -= 1;
  while (cTargY - cam.y > 2520) cam.y += 1;*/
  cam.x += (cTargX - cam.x) / ((15 * int(!instant)) + 1);
  cam.y += (cTargY - cam.y) / ((15 * int(!instant)) + 1);
  cam.x = gamemax(gamemin(cam.x, (-worldSizeX * 2) + 5120 * 2), (worldSizeX * 2) - 5120 * 3);
  cam.y = gamemax(gamemin(cam.y, (-worldSizeZ * 2) + 2880 * 2), (worldSizeZ * 2) - 2880 * 3);
}