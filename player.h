#ifndef playerh
#define playerh
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "window.h"
#include "helpers.h"
#include "controller.h"
#include "enemy.h"

class Enemy;
class BadFish;
class Shark;

struct PlayerInfo {
  int x;
  short y;
  int z;
  short xVel;
  short yVel;
  short zVel;
  bool onGround;
  bool flyAnim;
  bool airJump;
  bool airDash;
  bool lastJump;
  int faceAnim;
  bool flying;
  bool airBoost;
};

class Player {
  private:
    PlayerInfo info;
    SDL_Texture *img;
    SDL_Texture *face;
    Mix_Chunk *jump;
    Mix_Chunk *bump;
    Mix_Chunk *splash;
    Mix_Chunk *screenFinish;
    Mix_Chunk *jump2;
    Mix_Chunk *dash;
    Mix_Chunk *burn;
  public:
    Hitbox box;
    Player();
    void init(Window&, bool);
    int update(Controller*, bool, Particle*[particleCount], Enemy*[enemyCount], BadFish*[badFishCount], Shark*[sharkCount]);
    void render(Window&, Camera, Controller*, bool, char);
    void end();
    void updateCamera(Camera&, Controller*, bool);
    PlayerInfo getInfo() {return info;}
    void setInfo(PlayerInfo newInfo) {info = newInfo;}
};
#endif