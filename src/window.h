#ifndef windowh
#define windowh
#include <SDL2/SDL.h>
#define intResX 640
#define intResY 360

struct WindowInfo {
  int w;
  int h;
  int wRatio;
  int hRatio;
  SDL_Rect screenRect;
  int rW;
  int rH;
  bool lowDetail;
  bool lowRender;
};

class Window {
  private:
    SDL_Window *display;
    SDL_Renderer *renderer;
    bool toTexture;
    bool isScreenshot;
    SDL_Texture *winTexture;
    int renderTick;
  public:
    WindowInfo info;
    Window();
    ~Window();
    bool makeScreen(int, int, char[], bool, bool, bool);
    void killScreen();
    void resizeScreen(int, int, bool);
    void setLowRender(bool);
    void update();
    void end();
    void screenshot();
    SDL_Renderer* getRender() {return renderer;}
};
#endif
