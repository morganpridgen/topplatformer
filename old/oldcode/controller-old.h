#ifndef controllerh
#define controllerh
#include <SDL2/SDL.h>

struct Controller {
  char aX;
  char aY;
  char a2X;
  char a2Y;
  char b;
  bool home;
  unsigned char id;
  SDL_Joystick *jCtrl;
  SDL_GameController *cCtrl;
  SDL_Haptic *rumble;
};

bool ctrlUpdate(SDL_Event, Controller&);
Controller ctrlInit();
void ctrlRumble(Controller, float, unsigned short);
void ctrlClose(Controller);
#endif