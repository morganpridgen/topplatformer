#include "terrain.h"
#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
#include <SDL2/SDL.h>
#include <nlohmann/json.hpp>
#include "window.h"
#include "helpers.h"
#include "player.h"
#define hitboxSize 128
using json = nlohmann::json;

struct Color {
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

json terrain;
SDL_Texture *barrier = nullptr, *water = nullptr, *ground = nullptr, *goal = nullptr, *magnet = nullptr, *fire = nullptr, *fakeBarrier = nullptr;
bool secretRevealed = 0;

int typeAt(unsigned char x, unsigned char z) {
  return int(terrain["layout"][z][x]);
}

void setType(unsigned char x, unsigned char z, int type) {
  terrain["layout"][z][x] = type;
}

void terrainLoadImg(Window &win) {
  barrier = loadTexture("res/barrier.png", win);
  water = loadTexture("res/water.png", win);
  ground = loadTexture("res/ground.png", win);
  goal = loadTexture("res/goal.png", win);
  magnet = loadTexture("res/magnet.png", win);
  fire = loadTexture("res/fire.png", win);
  fakeBarrier = loadTexture("res/fakebarrier.png", win);
}

void unloadTerrain() {
  SDL_DestroyTexture(barrier);
  SDL_DestroyTexture(water);
  SDL_DestroyTexture(ground);
  SDL_DestroyTexture(goal);
  SDL_DestroyTexture(magnet);
  SDL_DestroyTexture(fire);
  SDL_DestroyTexture(fakeBarrier);
}

StartPos loadLevel(char *name, unsigned char screenNum) {
  secretRevealed = 0;
  std::fstream data;
  std::stringstream path;
  path << "res/levels/" << name << "/screen" << int(screenNum) << ".json";
  data.open(path.str().c_str(), std::ios::in);
  if (data) {
    data.seekg(0, data.end);
    int length = data.tellg() + 1L;
    data.seekg(0, data.beg);
    char *buffer = new char[length];
    memset(buffer, 0, length);
    data.read(buffer, length);
    data.close();
    terrain = json::parse(buffer);
    StartPos start = {(int(terrain["startpos"][0]) * 256) + 128 - worldSizeX, (int(terrain["startpos"][1]) * 256) + 128 - worldSizeZ, 1};
    return start;
  } else {
    StartPos start = {0, 0, 0};
    return start;
  }
}

Tile *terrainAt(short x, short y) {
  Tile *tileInfo = new Tile;
  tileInfo->type = typeAt(int(float(x + worldSizeX) / 256.0), int(float(y + worldSizeZ) / 256.0));
  tileInfo->top = Hitbox(x + (hitboxSize / 2), y + (hitboxSize / 2), 256, hitboxSize);
  tileInfo->bot = Hitbox(x + (hitboxSize / 2), y + 256 - (hitboxSize / 2), 256, hitboxSize);
  tileInfo->left = Hitbox(x + (hitboxSize / 2), y + (hitboxSize / 2), hitboxSize, 256);
  tileInfo->right = Hitbox(x + 256 - (hitboxSize / 2), y + (hitboxSize / 2), hitboxSize, 256);
  tileInfo->all = Hitbox(x, y, 256, 256);
  //SDL_Log("{%i, %i} %i\n", int(float(x + worldSizeX) / 256.0), int(float(y + worldSizeZ) / 256.0), tileInfo->type);
  return tileInfo;
}

void updateTerrain(Player &ply, Particle *particles[particleCount]) {
  PlayerInfo pInfo = ply.getInfo();
  unsigned char dirX;
  unsigned char dirZ;
  unsigned char tileX;
  unsigned char tileZ;
  unsigned short dist = 0;
  unsigned short minDist = -1;
  unsigned short maxDist = -1;
  ParticleInfo newInfo;
  for (unsigned char i = 0; i < 45; i++) {
    for (unsigned char j = 0; j < 80; j++) {
      switch (typeAt(j, i)) {
        case 3:
          if (getTimer() % 2 == 0) {
            for (unsigned short k = 0; k < particleCount; k++) {
              if (!particles[k]) {
                particles[k] = new Particle;
                ParticleInfo newInfo = {j * 256 + 127 - worldSizeX, 0, i * 256 + 127 - worldSizeZ, (rng() % 65) - 32, 0, (rng() % 65) - 32, rng() % 4};
                particles[k]->setInfo(newInfo);
                break;
              }
            }
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
          for (unsigned char k = 0; k < 4; k++) {
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
            if (i != tileZ || j != tileX) {
              if (typeAt(tileX, tileZ) == 1) {
                tileX = j + dirX;
                tileZ = i + dirZ;
                while (typeAt(tileX, tileZ) == 7) {
                  tileX += dirX;
                  tileZ += dirZ;
                }
                if (typeAt(tileX, tileZ) == 1) setType(tileX, tileZ, 7);
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
      }
    }
  }
  if (minDist < 128) secretRevealed = 1;
  if (minDist > 512) secretRevealed = 0;
}

void renderTerrain(Window &win, Camera cam, Player &ply) {
  PlayerInfo pInfo = ply.getInfo();
  SDL_Rect rect = {0, 0, 32 * win.info.wRatio, 32 * win.info.hRatio};
  SDL_Rect clip = {0, 0, 32, 32};
  int dist;
  SDL_Rect miniRect;
  for (unsigned char i = 0; i < 45; i++) {
    for (unsigned char j = 0; j < 80; j++) {
      rect.x = WORLDTOPIX(j * 256, cam.x - worldSizeX) * win.info.wRatio;
      rect.y = WORLDTOPIX(i * 256, cam.y - worldSizeZ) * win.info.hRatio;
      clip.x = 0;
      if (rect.x < -32 * win.info.wRatio || rect.y < -32 * win.info.hRatio || rect.x > intResX * win.info.wRatio || rect.y > intResY * win.info.hRatio) continue;
      Color color = {0, 0, 0};
      switch (typeAt(j, i)) {
        case 0:
          if (!win.info.lowDetail) clip.x = 32 * (((getTimer() / 4) + i + j) % 32 < 2);
          SDL_RenderCopy(win.getRender(), barrier, &clip, &rect);
          break;
        case 1:
          SDL_RenderCopy(win.getRender(), ground, NULL, &rect);
          break;
        case 2:
          clip.x = ((getTimer() / 4) % 4) * 32;
          SDL_RenderCopy(win.getRender(), water, &clip, &rect);
          break;
        case 3:
          SDL_RenderCopy(win.getRender(), ground, NULL, &rect);
          if ((getTimer() % 64) < 48) clip.x = 0;
          else clip.x = ((((getTimer() % 64) - 48) / 4) % 4) * 32 + 32;
          SDL_RenderCopy(win.getRender(), goal, &clip, &rect);
          break;
        case 4:
          /*SDL_RenderCopy(win.getRender(), ground, NULL, &rect);
          if (win.info.lowDetail) dist = 256;
          else dist = abs(gamemin(abs(pInfo.x - ((j * 256) - worldSizeX) - 128), abs(pInfo.z - ((i * 256) - worldSizeZ) - 128)));
          if (dist < 512) {
            clip.x = 0;
            if (dist < 256) clip.x = 32;
            SDL_RenderCopy(win.getRender(), magnet, &clip, &rect);
          }*/
          clip.x = 32 * secretRevealed;
          SDL_RenderCopy(win.getRender(), fakeBarrier, &clip, &rect);
          break;
        case 5:
          if (win.info.lowDetail) {
            setColorFromPalette(win, ((i * i + j - (getTimer() / 8)) / 2) % 2);
            SDL_RenderFillRect(win.getRender(), &rect);
            break;
          }
          for (unsigned char i = 0; i < 8; i++) {
            for (unsigned char j = 0; j < 8; j++) {
              setColorFromPalette(win, ((i * i + j - (getTimer() / 8)) / 2) % 2);
              miniRect = {rect.x + (i * 4 * win.info.wRatio), rect.y + (j * 4 * win.info.hRatio), 4 * win.info.wRatio, 4 * win.info.hRatio};
              SDL_RenderFillRect(win.getRender(), &miniRect);
            }
          }
          break;
        case 6:
          SDL_RenderCopy(win.getRender(), fire, &clip, &rect);
          break;
        case 7:
          SDL_RenderCopy(win.getRender(), ground, NULL, &rect);
          clip.x = 32 * ((rng() % 2) + 1);
          SDL_RenderCopy(win.getRender(), fire, &clip, &rect);
          break;
      }
    }
  }
}