#ifndef helpersh
#define helpersh
#include <SDL2/SDL.h>
#include "window.h"
#define worldSizeX 10240
#define worldSizeZ 5760
#define WORLDTOPIX(x, y) (float(x - y) / 8)
#define gamemax(a, b) (((a)>(b))?(b):(a))
#define gamemin(a, b) (((a)<(b))?(b):(a))
#define yP(x) (__builtin_expect(x, 1))
#define nP(x) (__builtin_expect(x, 0))
#define particleCount 4096
//#define screenshot
unsigned short rng();
SDL_Texture* loadTexture(const char[], Window&);
void updateTimer();
unsigned long getTimer();
void setColorFromPalette(Window &win, int color);

struct HitboxInfo {
  short xBot;
  short xTop;
  short yBot;
  short yTop;
};
class Hitbox {
  public:
    HitboxInfo box;
    Hitbox(short, short, short, short);
    Hitbox();
    bool collide(Hitbox);
    void changeBox(short, short, short, short);
};

struct Camera {
  int x;
  int y;
};

struct ParticleInfo {
  int x;
  int y;
  int z;
  short xVel;
  short yVel;
  short zVel;
  int color;
};

class Particle {
  private:
    ParticleInfo info;
    int timer;
    bool active;
  public:
    bool shiny;
    Particle();
    bool getActive() {return active;}
    ParticleInfo getInfo() {return info;}
    void setInfo(ParticleInfo newInfo) {info = newInfo;}
    void update();
    void render(Window&, Camera, bool);
};

void addParticle(Particle **, ParticleInfo, bool);
void addParticle(Particle **, ParticleInfo);

bool loadFonts();
void unloadFonts();
SDL_Surface *surfaceText(char*, bool, bool, int);
SDL_Texture *renderText(Window&, char*, bool, int);
SDL_Texture *renderTextBg(Window&, char*, bool, int);
SDL_Texture *renderTextBg(Window&, char*, bool, int, int);

void initPaths();
char *pathForData(const char *);
char *pathForSaves(const char *);
#endif