#ifndef terrainh
#define terrainh
#include <SDL2/SDL.h>
#include "window.h"
#include "helpers.h"
#include "player.h"
#include "enemy.h"
#define maxFish 16

struct StartPos {
  short x;
  short y;
  bool levelExists;
};
struct Tile {
  int type;
  /*Hitbox top;
  Hitbox bot;
  Hitbox left;
  Hitbox right;*/
  Hitbox all;
};

void terrainLoadRes(Window&);
void unloadTerrain();
StartPos loadLevel(char*, int);
Tile *terrainAt(short, short);
bool isSolidType(int);
bool isAirType(int);
void updateTerrain(Player&, bool, Particle*[particleCount], Enemy*[enemyCount], BadFish*[badFishCount], Shark*[sharkCount]);
void renderTerrain(Window&, Camera, Player&, bool, char);
bool getSwitch();
void setSwitch(bool, bool);
void setSwitch(bool);
void killSClock();

struct LevelScores {
  char valid;
  unsigned long highScore;
  unsigned char grade;
  unsigned int plays;
  bool assistUsed;
};
LevelScores getScoresForLevel(char[64]);
void setScoresForLevel(char[64], LevelScores);

struct FishInfo {
  int x;
  int z;
  bool dir;
};

class Fish {
  private:
    FishInfo info;
  public:
    FishInfo getInfo() {return info;}
    void setInfo(FishInfo newInfo) {info = newInfo;}
    void update(Player*);
    void render(Window&, Camera);
};
#endif