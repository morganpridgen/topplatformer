#ifndef controllerh
#define controllerh
#include <SDL2/SDL.h>

#define CTRL_ACTION_GAME_JUMP 0b1
#define CTRL_ACTION_GAME_RUN 0b10
#define CTRL_ACTION_GAME_SQUISH 0b100
#define CTRL_ACTION_GAME_PAUSE 0b1000

#define CTRL_ACTION_MENU_OK 0b1001
#define CTRL_ACTION_MENU_BACK 0b100
#define CTRL_ACTION_MENU_SPECIAL 0b10000
#define CTRL_ACTION_MENU_SUBMENU 0b100000

#define CTRL_ACTIONNAME_GAME_MOVE 1
#define CTRL_ACTIONNAME_GAME_LOOK 2
#define CTRL_ACTIONNAME_GAME_JUMP 3
#define CTRL_ACTIONNAME_GAME_RUN 4
#define CTRL_ACTIONNAME_GAME_SQUISH 5
#define CTRL_ACTIONNAME_GAME_PAUSE 6

#define CTRL_ACTIONNAME_MENU_MOVE 11
#define CTRL_ACTIONNAME_MENU_OK 12
#define CTRL_ACTIONNAME_MENU_BACK 13
#define CTRL_ACTIONNAME_MENU_SPECIAL 14
#define CTRL_ACTIONNAME_MENU_SUBMENU 15

#define CTRL_ACTIONNAME_SCREENSHOT 64

#define CTRL_TYPE_KEYBOARD 0b1
#define CTRL_TYPE_XINPUT 0b10
#define CTRL_TYPE_STEAMINPUT 0b100

struct Controller {
  char aX;
  char aY;
  char a2X;
  char a2Y;
  char b;
  bool home;
  bool screenshot;
  bool lastScreenshot;
  char id;
  int type;
  int lastType;
  SDL_Joystick *jCtrl;
  SDL_GameController *cCtrl;
  SDL_Haptic *rumble;
};
struct CtrlList {
  Controller *ctrls[8];
  int ctrlCount;
};

bool ctrlEvent(SDL_Event, Controller&);
void ctrlUpdate(CtrlList&);
void ctrlCheckForNew(SDL_Event, CtrlList&);
CtrlList ctrlInit(bool, bool);
void ctrlRumble(Controller&, float, int);
void ctrlSetRumble(bool);
void ctrlClose(CtrlList&);
char ctrlActionToChr(int);
SDL_Surface *ctrlGetGlyphForButton(char, int);
#endif