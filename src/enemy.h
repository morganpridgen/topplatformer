#ifndef enemyh
#define enemyh
#define enemyCount 64
#define badFishCount 32
#define sharkCount 32
#include "window.h"
#include "helpers.h"
#include "player.h"

class Player;

struct EnemyInfo {
  int x;
  int z;
  char xVel;
  char zVel;
  int moveTimer;
  int seed;
};

class Enemy {
  private:
    EnemyInfo info;
    bool active;
  public:
    Enemy();
    void update();
    void render(Window&, Camera);
    void tickRand();
    EnemyInfo getInfo() {return info;}
    void setInfo(EnemyInfo newInfo) {info = newInfo;}
    bool getActive() {return active;}
    Hitbox getHitbox() {
      Hitbox box;
      box.changeBox(info.x, info.z, 128, 128);
      return box;
    }
};

struct BadFishInfo {
  int x;
  int z;
  int shotTimer;
  bool playerClose;
};
struct BadFishProj {
  int x;
  int z;
  int targetX;
  int targetZ;
};

class BadFish {
  private:
    BadFishInfo info;
    BadFishProj shot;
  public:
    BadFish() {
      shot = {0, 0, 0, 0};
    }
    void update(Player&, Particle*[particleCount]);
    void render(Window&, Camera, bool);
    BadFishInfo getInfo() {return info;}
    void setInfo(BadFishInfo newInfo) {info = newInfo;}
    Hitbox getHitbox() {
      Hitbox box;
      if (shot.x == shot.targetX && shot.z == shot.targetZ) box.changeBox(info.x, info.z, 1, 1);
      else box.changeBox(shot.x, shot.z, 64, 64);
      return box;
    }
};

struct SharkInfo {
  int x;
  int y;
  int z;
  short xVel;
  short yVel;
  short zVel;
  bool firstHop;
  bool hiding;
};

class Shark {
  private:
    SharkInfo info;
  public:
    Shark() {
      info = {0, 0, 0, 0, 0, 0, 1, 1};
    }
    void update(Player&, Particle*[particleCount]);
    void render(Window&, Camera, bool);
    SharkInfo getInfo() {return info;}
    void setInfo(SharkInfo newInfo) {info = newInfo;}
};

bool enemyLoadRes(Window&);
void enemyFreeRes();

#endif