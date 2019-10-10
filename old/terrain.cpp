#include "terrain.h"
#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <nlohmann/json.hpp>
#include "window.h"
#include "helpers.h"
#include "player.h"
#define hitboxSize 128
#define levelW 80
#define levelH 45
#define LVL_Enabled 1
using json = nlohmann::json;

Fish *fish[maxFish];
SDL_Texture *fishImg = nullptr;
void genFish();
int switchClock = 0;
int clockDisable = 0;
float fbAngle = 0;

struct Color {
  int r;
  int g;
  int b;
};
struct Light {
  int x;
  int z;
  int type;
};

json jTerrain;
int terrain[levelH * levelW];
//SDL_Texture *barrier = nullptr, *water = nullptr, *ground = nullptr, *goal = nullptr, *magnet = nullptr, *fire = nullptr, *fakeBarrier = nullptr;
SDL_Texture *terrainImg = nullptr;
Mix_Chunk *switchPing = nullptr, *switchPong = nullptr;
bool secretRevealed = 0;
bool switchState = 0;

bool getSwitch() {
  return switchState;
}

void setSwitch(bool newSwitch, bool silent) {
  switchState = newSwitch;
  if (!silent) Mix_PlayChannel(-1, switchState ? switchPing : switchPong, 0);
}

void setSwitch(bool newSwitch) {
  setSwitch(newSwitch, 0);
}

void killSClock() {
  clockDisable = 256;
  switchClock = 1;
  Mix_PlayChannel(-1, switchPong, 0);
}

int typeAt(int x, int z) {
  if (nP(x < 0 || x > 79 || z < 0 || z > 44)) return 0;
  return *(terrain + z * levelW + x);
}

void setType(int x, int z, int type) {
  if (nP(x < 0 || x > levelW || z < 0 || z > levelH)) return;
  *(terrain + z * levelW + x) = type;
}

void terrainLoadRes(Window &win) {
  /*barrier = loadTexture("res/imgs/barrier.png", win);
  water = loadTexture("res/imgs/water.png", win);
  ground = loadTexture("res/imgs/ground.png", win);
  goal = loadTexture("res/imgs/goal.png", win);
  magnet = loadTexture("res/imgs/magnet.png", win);
  fire = loadTexture("res/imgs/fire.png", win);
  fakeBarrier = loadTexture("res/imgs/fakebarrier.png", win);*/
  fishImg = loadTexture(pathForData("imgs/fish.png"), win);
  char *paths[] = {"imgs/barrier.png", "imgs/ground.png", "imgs/water.png", "imgs/goal.png", "imgs/fakebarrier.png", "imgs/fire.png", "imgs/switch.png", "imgs/switchblock.png", "imgs/fan.png"};
  SDL_Surface *tmpImg = SDL_CreateRGBSurfaceWithFormat(0, 256, 512, 32, SDL_PIXELFORMAT_RGBA32);
  for (int i = 0; i < 9; i++) {
    SDL_Rect rect = {0, 32 * i, 32, 32};
    SDL_Surface *in = IMG_Load(pathForData(paths[i]));
    SDL_BlitSurface(in, NULL, tmpImg, &rect);
    SDL_FreeSurface(in);
  }
  terrainImg = SDL_CreateTextureFromSurface(win.getRender(), tmpImg);
  SDL_FreeSurface(tmpImg);
  switchPing = Mix_LoadWAV(pathForData("sfx/switchping.wav"));
  switchPong = Mix_LoadWAV(pathForData("sfx/switchpong.wav"));
}

void unloadTerrain() {
  /*SDL_DestroyTexture(barrier);
  SDL_DestroyTexture(water);
  SDL_DestroyTexture(ground);
  SDL_DestroyTexture(goal);
  SDL_DestroyTexture(magnet);
  SDL_DestroyTexture(fire);
  SDL_DestroyTexture(fakeBarrier);*/
  SDL_DestroyTexture(fishImg);
  SDL_DestroyTexture(terrainImg);
  Mix_FreeChunk(switchPing);
  Mix_FreeChunk(switchPong);
}

StartPos loadLevel(char *name, int screenNum) {
  for (int i = 0; i < maxFish; i++) {
    if (fish[i]) delete fish[i];
    fish[i] = nullptr;
  }
  memset(terrain, 0, sizeof(terrain));
  secretRevealed = 0;
  setSwitch(0, 1);
  clockDisable = 0;
  fbAngle = 0;
  std::fstream data;
  std::stringstream path;
  path << "levels/" << name << "/screen" << int(screenNum) << ".json";
  data.open(pathForData(path.str().c_str()), std::ios::in);
  if (data) {
    data.seekg(0, data.end);
    int length = data.tellg() + 1L;
    data.seekg(0, data.beg);
    char *buffer = new char[length];
    memset(buffer, 0, length);
    data.read(buffer, length);
    data.close();
    jTerrain = json::parse(buffer);
    for (int i = 0; i < levelH; i++) {
      for (int j = 0; j < levelW; j++) {
        terrain[i * levelW + j] = int(jTerrain["layout"][i][j]);
      }
    }
    genFish();
    StartPos start = {(int(jTerrain["startpos"][0]) * 256) + 128 - worldSizeX, (int(jTerrain["startpos"][1]) * 256) + 128 - worldSizeZ, 1};
    return start;
  } else {
    #if LVL_Enabled == 1
      std::stringstream newPath;
      newPath << "levels/" << name << "/screen" << int(screenNum) << ".lvl";
      data.open(pathForData(newPath.str().c_str()), std::ios::in | std::ios::binary);
      if (data) {
        data.seekg(0, data.end);
        int length = data.tellg() + 1L;
        data.seekg(0, data.beg);
        char *buffer = new char[length];
        memset(buffer, 0, length);
        data.read(buffer, length);
        data.close();
        if (buffer[0] != 'C' || buffer[1] != 'M') {
          StartPos start = {0, 0, 0};
          return start;
        }
        int palette[8];
        for (int i = 0; i < 8; i++) {
          palette[i] = buffer[i + 2];
        }
        StartPos start = {buffer[10] * 256 + 128 - worldSizeX, buffer[11] * 256 + 128 - worldSizeZ, 1};
        for (int i = 0; i < levelH; i++) {
          for (int j = 0; j < levelW; j++) {
            terrain[i * levelW + j] = palette[buffer[i * levelW + j + 12]];
          }
        }
        genFish();
        return start;
      } else {
        StartPos start = {0, 0, 0};
        return start;
      }
    #else
      StartPos start = {0, 0, 0};
      return start;
    #endif
  }
}

Tile *terrainAt(short x, short y) {
  Tile *tileInfo = new Tile;
  tileInfo->type = typeAt(int(float(x + worldSizeX) / 256.0), int(float(y + worldSizeZ) / 256.0));
  /*tileInfo->top = Hitbox(x + (hitboxSize / 2), y + (hitboxSize / 2), 256, hitboxSize);
  tileInfo->bot = Hitbox(x + (hitboxSize / 2), y + 256 - (hitboxSize / 2), 256, hitboxSize);
  tileInfo->left = Hitbox(x + (hitboxSize / 2), y + (hitboxSize / 2), hitboxSize, 256);
  tileInfo->right = Hitbox(x + 256 - (hitboxSize / 2), y + (hitboxSize / 2), hitboxSize, 256);*/
  tileInfo->all = Hitbox(x, y, 256, 256);
  //SDL_Log("{%i, %i} %i\n", int(float(x + worldSizeX) / 256.0), int(float(y + worldSizeZ) / 256.0), tileInfo->type);
  return tileInfo;
}

bool isSolidType(int type) {
  return type == 0 || type == 3 || type == 4 || type == 6 || type == 11 || type == 12 || type == 13 || type == 15 || type == 17 || type == 19;
}

bool isAirType(int type) {
  return type == 1 || type == 2 || type == 5 || type == 7;
}

void updateTerrain(Player &ply, bool inWater, Particle *particles[particleCount], Enemy *enemies[enemyCount], BadFish *badFish[badFishCount], Shark *sharks[sharkCount]) {
  PlayerInfo pInfo = ply.getInfo();
  char dirX;
  char dirZ;
  int tileX;
  int tileZ;
  int dist = 0;
  int minDist = -1;
  int maxDist = -1;
  EnemyInfo newEInfo;
  BadFishInfo newBInfo;
  SharkInfo newSInfo;
  for (int i = 0; i < levelH; i++) {
    for (int j = 0; j < levelW; j++) {
      switch (typeAt(j, i)) {
        case 3:
          if (getTimer() % 2 == 0) {
            addParticle(particles, {j * 256 + 127 - worldSizeX, 0, i * 256 + 127 - worldSizeZ, (rng() % 65) - 32, 0, (rng() % 65) - 32, rng() % 4}, 1);
          }
          break;
        case 4:
          dist = abs(gamemin(abs(pInfo.x - ((j * 256) - worldSizeX) - 128), abs(pInfo.z - ((i * 256) - worldSizeZ) - 128)));
          if (dist > maxDist) maxDist = dist;
          if (dist < minDist) minDist = dist;
          break;
        case 6:
          if (getTimer() % 8) continue;
          dirX = 0;
          dirZ = 0;
          tileX = j;
          tileZ = i;
          for (int k = 0; k < 4; k++) {
            dirX = 0;
            dirZ = 0;
            tileX = j;
            tileZ = i;
            if (k == 0) dirX = -1;
            if (k == 1) dirX = 1;
            if (k == 2) dirZ = -1;
            if (k == 3) dirZ = 1;
            tileX += dirX;
            tileZ += dirZ;
            while (typeAt(tileX, tileZ) == 1 || typeAt(tileX, tileZ) == 7) {
              tileX += dirX;
              tileZ += dirZ;
            }
            tileX -= dirX;
            tileZ -= dirZ;
            if (yP(i != tileZ || j != tileX)) {
              if (typeAt(tileX, tileZ) == 1) {
                tileX = j + dirX;
                tileZ = i + dirZ;
                while (typeAt(tileX, tileZ) == 7) {
                  tileX += dirX;
                  tileZ += dirZ;
                }
                if (typeAt(tileX, tileZ) == 1) setType(tileX, tileZ, 7);
                #ifndef screenshot
                  for (int l = 0; l < 4; l++) addParticle(particles, {j * 256 + 127 - worldSizeX + 128 * dirX, 0, i * 256 + 127 - worldSizeZ + 128 * dirZ, dirX * 64 + ((rng() % 33) - 16), 0, dirZ * 64 + ((rng() % 33) - 16), rng() % 2}, 1);
                #endif
              } else if (typeAt(tileX, tileZ) == 7) {
                tileX = j + dirX;
                tileZ = i + dirZ;
                while (typeAt(tileX, tileZ) == 1) {
                  tileX += dirX;
                  tileZ += dirZ;
                }
                if (typeAt(tileX, tileZ) == 7) setType(tileX, tileZ, 1);
              }
            }
          }
          break;
        case 7:
          addParticle(particles, {j * 256 + 127 - worldSizeX, 0, i * 256 + 127 - worldSizeZ, ((rng() % 33) - 16), 0, ((rng() % 33) - 16), rng() % 2}, 1);
          break;
        case 8:
          setType(j, i, 1);
          newEInfo = {j * 256 + 127 - worldSizeX, i * 256 + 127 - worldSizeZ, 0, 0, 64, (j + i) & 0b1111};
          for (int k = 0; k < enemyCount; k++) {
            if (!enemies[k]) {
              enemies[k] = new Enemy;
              enemies[k]->setInfo(newEInfo);
              break;
            }
          }
          break;
        case 9:
          setType(j, i, 2);
          newBInfo = {j * 256 + 127 - worldSizeX, i * 256 + 127 - worldSizeZ, 16};
          for (int k = 0; k < badFishCount; k++) {
            if (!badFish[k]) {
              badFish[k] = new BadFish;
              badFish[k]->setInfo(newBInfo);
              break;
            }
          }
          break;
        case 10:
          setType(j, i, 2);
          newSInfo = {j * 256 + 127 - worldSizeX, 0, i * 256 + 127 - worldSizeZ, 0, 0, 0, 1, 1};
          for (int k = 0; k < sharkCount; k++) {
            if (!sharks[k]) {
              sharks[k] = new Shark;
              sharks[k]->setInfo(newSInfo);
              break;
            }
          }
          break;
        case 14:
          if (clockDisable) {
            clockDisable--;
            if (clockDisable < 64 && clockDisable % 16 == 0) Mix_PlayChannel(-1, switchPing, 0);
          } else {
            switchClock++;
            if (switchClock % 64 == 0) setSwitch(!switchState);
          }
          break;
        case 17: {
          if (pInfo.flyAnim) break;
          int pX = int(float(pInfo.x + worldSizeX) / 256.0);
          int pZ = int(float(pInfo.z + worldSizeZ) / 256.0);
          int k = 0;
          if (pX == j) {
            if (pZ < i) {
              k = i - 1;
              while (isAirType(typeAt(j, k))) {
                if (k == pZ) {
                  pInfo.zVel -= 2;
                  pInfo.z -= 4;
                  ply.setInfo(pInfo);
                  break;
                }
                k--;
              }
            }
            if (pZ > i) {
              k = i + 1;
              while (isAirType(typeAt(j, k))) {
                if (k == pZ) {
                  pInfo.zVel += 2;
                  pInfo.z += 4;
                  ply.setInfo(pInfo);
                  break;
                }
                k++;
              }
            }
          }
          if (pZ == i) {
            if (pX < j) {
              k = j - 1;
              while (isAirType(typeAt(k, i))) {
                if (k == pX) {
                  pInfo.xVel -= 2;
                  pInfo.x -= 4;
                  ply.setInfo(pInfo);
                  break;
                }
                k--;
              }
            }
            if (pX > j) {
              k = j + 1;
              while (isAirType(typeAt(k, i))) {
                if (k == pX) {
                  pInfo.xVel += 2;
                  pInfo.x += 4;
                  ply.setInfo(pInfo);
                  break;
                }
                k++;
              }
            }
          }
          break;
        }
        case 19: {
          float oX = 64.0 * cos(fbAngle - (3.14 / 64.0));
          float oZ = 64.0 * sin(fbAngle - (3.14 / 64.0));
          float fX = j * 256 + 127 - worldSizeX;
          float fZ = i * 256 + 127 - worldSizeZ;
          for (int k = 0; k < 12; k++) {
            fX += oX;
            fZ += oZ;
            if (typeAt(round((fX + float(worldSizeX) - 127) / 256.0), round((fZ + float(worldSizeZ) - 127) / 256.0)) == 7) setType(round((fX + float(worldSizeX) - 127) / 256.0), round((fZ + float(worldSizeZ) - 127) / 256.0), 1);
          }
          oX = 64.0 * cos(fbAngle);
          oZ = 64.0 * sin(fbAngle);
          fX = j * 256 + 127 - worldSizeX;
          fZ = i * 256 + 127 - worldSizeZ;
          for (int k = 0; k < 12; k++) {
            fX += oX;
            fZ += oZ;
            if (typeAt(round((fX + float(worldSizeX) - 127) / 256.0), round((fZ + float(worldSizeZ) - 127) / 256.0)) == 1) setType(round((fX + float(worldSizeX) - 127) / 256.0), round((fZ + float(worldSizeZ) - 127) / 256.0), 7);
          }
          break;
        }
      }
    }
  }
  if (minDist < 128) secretRevealed = 1;
  if (minDist > 512) secretRevealed = 0;
  for (int i = 0; i < maxFish; i++) {
    if (fish[i]) fish[i]->update(inWater ? &ply : NULL);
  }
  fbAngle += 3.14 / 96.0;
  while (fbAngle > 6.28) fbAngle -= 6.28;
}

void renderTerrain(Window &win, Camera cam, Player &ply, bool simpleEffects, char layer) {
  PlayerInfo pInfo = ply.getInfo();
  SDL_Rect rect = {0, 0, 32 * win.info.wRatio, 32 * win.info.hRatio};
  SDL_Rect clip = {0, 0, 32, 32};
  SDL_Rect miniRect;
  Light lights[192];
  memset(lights, 0, sizeof(lights));
  int i = 0;
  int lowY = 0, highY = levelH;
  while (1) {
    int pos = WORLDTOPIX(i * 256, cam.y - worldSizeZ);
    if (layer == -1) pos += 8;
    if (layer == 1) pos -= 16;
    if (pos < -64) lowY = i;
    highY = i;
    if (pos > intResY + 32) break;
    i++;
  }
  i = 0;
  int lowX = 0, highX = levelW;
  while (1) {
    int pos = WORLDTOPIX(i * 256, cam.x - worldSizeX);
    if (pos < -32) lowX = i;
    highX = i;
    if (pos > intResX) break;
    i++;
  }
  for (int i = lowY; i < highY; i++) {
    rect.y = WORLDTOPIX(i * 256, cam.y - worldSizeZ) * win.info.hRatio;
    if (layer == -1) rect.y += 8 * win.info.hRatio;
    if (layer == 1) rect.y -= 16 * win.info.hRatio;
    //if (rect.y < -32 * win.info.hRatio || rect.y > intResY * win.info.hRatio) continue;
    for (int j = lowX; j < highX; j++) {
      rect.x = WORLDTOPIX(j * 256, cam.x - worldSizeX) * win.info.wRatio;
      clip.x = 0;
      //if (rect.x < -32 * win.info.wRatio || rect.x > intResX * win.info.wRatio) continue;
      /*if (levelFade && (i + j + (getTimer() / 2)) % levelFade == 0) {
        if (win.info.lowDetail) {
          setColorFromPalette(win, (i * i + j - (getTimer() / 8)) % 4);
          SDL_RenderFillRect(win.getRender(), &rect);
          break;
        }
        for (int i = 0; i < 8; i++) {
          for (int j = 0; j < 8; j++) {
            setColorFromPalette(win, (i * i + j - (getTimer() / 8)) % 4);
            miniRect = {rect.x + (i * 4 * win.info.wRatio), rect.y + (j * 4 * win.info.hRatio), 4 * win.info.wRatio, 4 * win.info.hRatio};
            SDL_RenderFillRect(win.getRender(), &miniRect);
          }
        }
      } else */
      switch (typeAt(j, i)) {
        case 0: {
          if (layer != 1) break;
          clip.y = 0;
          if (!win.info.lowDetail) clip.x = 32 * (((getTimer() / 4) + i + j) % 32 < 2);
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          SDL_Rect newRect = {rect.x, rect.y + 32 * win.info.hRatio, 32 * win.info.wRatio, 16 * win.info.hRatio};
          SDL_Rect newClip = {64, 16 * (((getTimer() / 4) + i + j) % 32 < 2), 32, 16};
          SDL_RenderCopy(win.getRender(), terrainImg, &newClip, &newRect);
          int floorType = typeAt(j, i + 1);
          if (isAirType(floorType)) {
            for (int k = 0; k < 192; k++) {
              if ((*(lights + k)).x == 0 && (*(lights + k)).z == 0) {
                (*(lights + k)) = {j, i, 0};
                break;
              }
            }
          }
          break;
        }
        case 1:
          if (layer) break;
          clip.y = 32;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          break;
        case 2: {
          if (layer != -1) break;
          clip.y = 64;
          clip.x = ((getTimer() / 4) % 4) * 32;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          if (typeAt(j, i - 1) == 2 || typeAt(j, i - 1) == 16) break;
          SDL_Rect newRect = {rect.x, rect.y - 8 * win.info.hRatio, 32 * win.info.wRatio, 8 * win.info.hRatio};
          SDL_Rect newClip = {128, ((getTimer() / 16) % 4) * 8 + 64, 32, 8};
          SDL_RenderCopy(win.getRender(), terrainImg, &newClip, &newRect);
          break;
        }
        case 3:
          if (layer) break;
          clip.y = 32;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          clip.y = 96;
          if ((getTimer() % 64) < 48) clip.x = 0;
          else clip.x = ((((getTimer() % 64) - 48) / 4) % 4) * 32 + 32;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          break;
        case 4: {
          /*SDL_RenderCopy(win.getRender(), ground, NULL, &rect);
          if (win.info.lowDetail) dist = 256;
          else dist = abs(gamemin(abs(pInfo.x - ((j * 256) - worldSizeX) - 128), abs(pInfo.z - ((i * 256) - worldSizeZ) - 128)));
          if (dist < 512) {
            clip.x = 0;
            if (dist < 256) clip.x = 32;
            SDL_RenderCopy(win.getRender(), magnet, &clip, &rect);
          }*/
          if (secretRevealed ? layer : layer != 1) break;
          clip.x = 32 * secretRevealed;
          clip.y = 128;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          SDL_Rect newRect = {rect.x, rect.y + 32 * win.info.hRatio, 32 * win.info.wRatio, 16 * win.info.hRatio};
          SDL_Rect newClip = {64, 0, 32, 16};
          SDL_RenderCopy(win.getRender(), terrainImg, &newClip, &newRect);
          break;
        }
        case 5:
          if (layer) break;
          if (win.info.lowDetail) {
            setColorFromPalette(win, ((i * i + j - (getTimer() / 8)) / 2) % 2);
            SDL_RenderFillRect(win.getRender(), &rect);
            break;
          }
          for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
              setColorFromPalette(win, ((i * i + j - (getTimer() / 8)) / 2) % 2);
              miniRect = {rect.x + (i * 4 * win.info.wRatio), rect.y + (j * 4 * win.info.hRatio), 4 * win.info.wRatio, 4 * win.info.hRatio};
              SDL_RenderFillRect(win.getRender(), &miniRect);
            }
          }
          break;
        case 6: {
          if (layer != 1) break;
          clip.y = 160;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          SDL_Rect newRect = {rect.x, rect.y + 32 * win.info.hRatio, 32 * win.info.wRatio, 16 * win.info.hRatio};
          SDL_Rect newClip = {96, 160, 32, 16};
          SDL_RenderCopy(win.getRender(), terrainImg, &newClip, &newRect);
          if (isAirType(typeAt(j, i + 1))) {
            for (int k = 0; k < 192; k++) {
              if ((*(lights + k)).x == 0 && (*(lights + k)).z == 0) {
                (*(lights + k)) = {j, i, 0};
                break;
              }
            }
          }
          break;
        }
        case 7: {
          if (layer) break;
          clip.y = 32;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          clip.y = 160;
          clip.x = 32 * ((rng() % 2) + 1);
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          /*int borders = 0;
          borders += (typeAt(j - 1, i) == 7);
          borders += (typeAt(j + 1, i) == 7);
          borders += (typeAt(j, i - 1) == 7);
          borders += (typeAt(j, i + 1) == 7);
          if (borders > 1) break;
          for (int k = 0; k < 192; k++) {
            if ((*(lights + k)).x == 0 && (*(lights + k)).z == 0) {
              (*(lights + k)) = {j, i, 7};
              break;
            }
          }*/
          break;
        }
        case 11:
          if (layer) break;
          clip.y = 32;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          clip.x = (64 * switchState) + (32 * ((getTimer() / 8) % 2));
          clip.y = 192;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          break;
        case 12: {
          if (switchState ? layer : layer != 1) break;
          clip.x = 32 * switchState;
          clip.y = 224;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          if (layer != 1) break;
          SDL_Rect newRect = {rect.x, rect.y + 32 * win.info.hRatio, 32 * win.info.wRatio, 16 * win.info.hRatio};
          SDL_Rect newClip = {128, 224, 32, 16};
          SDL_RenderCopy(win.getRender(), terrainImg, &newClip, &newRect);
          break;
        }
        case 13: {
          if (switchState ? layer != 1 : layer) break;
          clip.x = 64 + 32 * !switchState;
          clip.y = 224;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          if (layer != 1) break;
          SDL_Rect newRect = {rect.x, rect.y + 32 * win.info.hRatio, 32 * win.info.wRatio, 16 * win.info.hRatio};
          SDL_Rect newClip = {128, 240, 32, 16};
          SDL_RenderCopy(win.getRender(), terrainImg, &newClip, &newRect);
          break;
        }
        case 14:
          if (layer) break;
          clip.x = 128;
          clip.y = 192;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          break;
        case 15:
          if (layer) break;
          clip.x = 160;
          clip.y = 192;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          break;
        case 16: {
          if (layer != -1) break;
          clip.y = 256;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          clip.x = 32 + 32 * ((getTimer() / 2) % 4);
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          if (typeAt(j, i - 1) == 2 || typeAt(j, i - 1) == 16) break;
          SDL_Rect newRect = {rect.x, rect.y - 8 * win.info.hRatio, 32 * win.info.wRatio, 8 * win.info.hRatio};
          SDL_Rect newClip = {160, 288, 32, 8};
          SDL_RenderCopy(win.getRender(), terrainImg, &newClip, &newRect);
          break;
        }
        case 17: {
          if (layer != 1) break;
          clip.y = 256;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          clip.x = 32 + 32 * ((getTimer() / 2) % 4);
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          SDL_Rect newRect = {rect.x, rect.y + 32 * win.info.hRatio, 32 * win.info.wRatio, 16 * win.info.hRatio};
          SDL_Rect newClip = {160, 256, 32, 16};
          SDL_RenderCopy(win.getRender(), terrainImg, &newClip, &newRect);
          break;
        }
        case 18:
          if (layer) break;
          if (win.info.lowDetail) {
            setColorFromPalette(win, ((i + j - (getTimer() / 8)) / 2) % 2 + 2);
            SDL_RenderFillRect(win.getRender(), &rect);
            break;
          }
          for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
              setColorFromPalette(win, ((i + j - (getTimer() / 8)) / 2) % 2 + 2);
              miniRect = {rect.x + (i * 4 * win.info.wRatio), rect.y + (j * 4 * win.info.hRatio), 4 * win.info.wRatio, 4 * win.info.hRatio};
              SDL_RenderFillRect(win.getRender(), &miniRect);
            }
          }
          break;
        case 19: {
          if (layer != 1) break;
          clip.y = 160;
          SDL_RenderCopy(win.getRender(), terrainImg, &clip, &rect);
          SDL_Rect newRect = {rect.x, rect.y + 32 * win.info.hRatio, 32 * win.info.wRatio, 16 * win.info.hRatio};
          SDL_Rect newClip = {96, 160, 32, 16};
          SDL_RenderCopy(win.getRender(), terrainImg, &newClip, &newRect);
          if (isAirType(typeAt(j, i + 1))) {
            for (int k = 0; k < 192; k++) {
              if ((*(lights + k)).x == 0 && (*(lights + k)).z == 0) {
                (*(lights + k)) = {j, i, 0};
                break;
              }
            }
          }
          break;
        }
      }
    }
  }
  if (!simpleEffects) {
    for (int i = 0; i < 192; i++) {
      if ((*(lights + i)).x != 0 || (*(lights + i)).z != 0) {
        switch ((*(lights + i)).type) {
          case 0:
            setColorFromPalette(win, 1);
            for (int j = 0; j < 32 * win.info.wRatio; j++) {
              for (int k = 0; k < 16 * win.info.hRatio; k++) {
                if (int(WORLDTOPIX((*(lights + i)).x * 256, cam.x - worldSizeX) * win.info.wRatio + j + WORLDTOPIX(((*(lights + i)).z + 1) * 256, cam.y - worldSizeZ) * win.info.hRatio + k + getTimer()) % 2 == 0) SDL_RenderDrawPoint(win.getRender(), WORLDTOPIX((*(lights + i)).x * 256, cam.x - worldSizeX) * win.info.wRatio + j, WORLDTOPIX(((*(lights + i)).z + 1) * 256, cam.y - worldSizeZ) * win.info.hRatio + k);
              }
            }
            break;
          case 7: {
            int x = (*(lights + i)).x, z = (*(lights + i)).z;
            setColorFromPalette(win, 3);
            for (int j = 0; j < 48 * win.info.wRatio; j++) {
              for (int k = 0; k < 48 * win.info.hRatio; k++) {
                if (int(WORLDTOPIX((*(lights + i)).x * 256 - 64, cam.x - worldSizeX) * win.info.wRatio + j + WORLDTOPIX((*(lights + i)).z * 256 - 64, cam.y - worldSizeZ) * win.info.hRatio + k + getTimer()) % 2 == 0) SDL_RenderDrawPoint(win.getRender(), WORLDTOPIX((*(lights + i)).x * 256 - 64, cam.x - worldSizeX) * win.info.wRatio + j, WORLDTOPIX((*(lights + i)).z * 256 - 64, cam.y - worldSizeZ) * win.info.hRatio + k);
              }
            }
            break;
            }
        }
      }
    }
  }
  if (layer == -1) {
    for (int i = 0; i < maxFish; i++) {
      if (fish[i]) fish[i]->render(win, cam);
    }
  }
}

LevelScores getScoresForLevel(char levelName[64]) {
  LevelScores out = {'x', 359999, 0, 0, 1};
  std::fstream data;
  std::stringstream path;
  path << levelName << ".sav";
  data.open(pathForSaves(path.str().c_str()), std::ios::in | std::ios::binary);
  if (data) data.read((char*)&out, sizeof(out));
  data.close();
  return out;
}

void setScoresForLevel(char levelName[64], LevelScores save) {
  std::fstream data;
  std::stringstream path;
  path << levelName << ".sav";
  data.open(pathForSaves(path.str().c_str()), std::ios::out | std::ios::binary);
  if (data) data.write((char*)&save, sizeof(save));
  data.close();
}

void Fish::update(Player *ply) {
  if (ply) {
    info.x += (ply->getInfo().x - info.x) / 16;
    info.z += (ply->getInfo().z - info.z) / 16;
  } else {
    if (info.dir) info.x += 8;
    else info.x -= 8;
    Tile *tileInfo = terrainAt(info.x, info.z);
    if (tileInfo->type != 2) info.dir = !info.dir;
    delete tileInfo;
  }
}

void Fish::render(Window &win, Camera cam) {
  Tile *tileInfo = terrainAt(info.x, info.z);
  if (tileInfo->type == 2) {
    SDL_Rect rect = {(WORLDTOPIX(info.x, cam.x) - 3) * win.info.wRatio, (WORLDTOPIX(info.z, cam.y) - 3 + 4 * sin(float(getTimer()) / 16)) * win.info.hRatio, 8 * win.info.wRatio, 8 * win.info.hRatio};
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    if (info.dir) flip = SDL_FLIP_HORIZONTAL;
    SDL_RenderCopyEx(win.getRender(), fishImg, NULL, &rect, 0, NULL, flip);
  }
  delete tileInfo;
}

void genFish() {
  #ifdef screenshot
    return;
  #endif
  int posX;
  int posZ;
  for (int i = 0; i < maxFish; i++) {
    posX = -1;
    posZ = -1;
    int noWater = 0;
    while (typeAt(posX, posZ) != 2) {
      posX = rng() % levelW;
      posZ = rng() % levelH;
      noWater++;
      if (noWater > 16) break;
    }
    if (typeAt(posX, posZ) != 2) continue;
    if (!fish[i]) {
      fish[i] = new Fish;
      FishInfo newInfo = {posX * 256 + 127 - worldSizeX, posZ * 256 + 127 - worldSizeZ, rng() % 2};
      fish[i]->setInfo(newInfo);
    }
  }
}