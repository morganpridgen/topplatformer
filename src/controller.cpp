#include "controller.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "helpers.h"
#ifdef _WIN32
  #include <string.h>
#endif

bool keys = 0;
bool vibrate = 0;
SDL_Scancode keyUp = SDL_SCANCODE_W, keyDown = SDL_SCANCODE_S, keyLeft = SDL_SCANCODE_A, keyRight = SDL_SCANCODE_D, keyJump = SDL_SCANCODE_SPACE, keyDash  = SDL_SCANCODE_LSHIFT, keyPause = SDL_SCANCODE_RETURN, keyBack = SDL_SCANCODE_ESCAPE, keySubMenu = SDL_SCANCODE_TAB, keySpecial = SDL_SCANCODE_E;
int controllerTypes = 0;
SDL_Surface *ctrlKeyboard = nullptr;
SDL_Surface *ctrlXinput = nullptr;

bool ctrlEvent(SDL_Event e, Controller &ctrl) {
  /*if (keys && ctrl.id == 0) {
    if (e.type == SDL_KEYDOWN) {
      ctrl.type = CTRL_TYPE_KEYBOARD;
      if (e.key.keysym.sym == keyUp) ctrl.aY = -127;
      if (e.key.keysym.sym == keyDown) ctrl.aY = 127;
      if (e.key.keysym.sym == keyLeft) ctrl.aX = -127;
      if (e.key.keysym.sym == keyRight) ctrl.aX = 127;
      if (e.key.keysym.sym == keyJump) ctrl.b |= CTRL_ACTION_GAME_JUMP;
      if (e.key.keysym.sym == keyDash) ctrl.b |= CTRL_ACTION_GAME_RUN;
      if (e.key.keysym.sym == keyPause) ctrl.b |= CTRL_ACTION_GAME_PAUSE;
    }
    if (e.type == SDL_KEYUP) {
      if (e.key.keysym.sym == keyUp && ctrl.aY == -127) ctrl.aY = 0;
      if (e.key.keysym.sym == keyDown && ctrl.aY == 127) ctrl.aY = 0;
      if (e.key.keysym.sym == keyLeft && ctrl.aX == -127) ctrl.aX = 0;
      if (e.key.keysym.sym == keyRight && ctrl.aX == 127) ctrl.aX = 0;
      if (e.key.keysym.sym == keyJump) ctrl.b &= ~CTRL_ACTION_GAME_JUMP;
      if (e.key.keysym.sym == keyDash) ctrl.b &= ~CTRL_ACTION_GAME_RUN;
      if (e.key.keysym.sym == keyPause) ctrl.b &= ~CTRL_ACTION_GAME_PAUSE;
    }
  }*/
  if (keys && ctrl.id == 0) {
    if (e.type == SDL_KEYDOWN) ctrl.type = CTRL_TYPE_KEYBOARD;
  }
  if (ctrl.cCtrl) {
    if (e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.which == ctrl.id) ctrl.type = CTRL_TYPE_XINPUT;
  }
  /*if (ctrl.jCtrl) {
    if (ctrl.cCtrl) {
      if (e.type == SDL_CONTROLLERAXISMOTION) {
        if (e.caxis.which == ctrl.id) {
          if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) ctrl.aX = e.caxis.value / 256;
          if (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) ctrl.aY = e.caxis.value / 256;
          if (e.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTX) ctrl.a2X = e.caxis.value / 256;
          if (e.caxis.axis == SDL_CONTROLLER_AXIS_RIGHTY) ctrl.a2Y = e.caxis.value / 256;
        }
      }
      if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        if (e.cbutton.which == ctrl.id) {
          ctrl.b |= (1 << (e.cbutton.button - (e.cbutton.button > 4) - 2 * (e.cbutton.button > 6)) & ((1 << 8) - 1));
          if (e.cbutton.button == 5) ctrl.home = 1;
          if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) ctrl.aY = -127;
          if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) ctrl.aY = 127;
          if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) ctrl.aX = -127;
          if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ctrl.aX = 127;
        }
      }
      if (e.type == SDL_CONTROLLERBUTTONUP) {
        if (e.cbutton.which == ctrl.id) {
          ctrl.b &= ~((1 << (e.cbutton.button - (e.cbutton.button > 4) - 2 * (e.cbutton.button > 6)) & ((1 << 8) - 1)));
          if (e.cbutton.button == 5) ctrl.home = 0;
          if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP && ctrl.aY == -127) ctrl.aY = 0;
          if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN && ctrl.aY == 127) ctrl.aY = 0;
          if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT && ctrl.aX == -127) ctrl.aX = 0;
          if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT && ctrl.aX == 127) ctrl.aX = 0;
        }
      }
    } else {
      if (e.type == SDL_JOYAXISMOTION) {
        if (e.jaxis.which == ctrl.id) {
          if (e.jaxis.axis == 0) ctrl.aX = e.jaxis.value / 256;
          if (e.jaxis.axis == 1) ctrl.aY = e.jaxis.value / 256;
          if (e.jaxis.axis == 2) ctrl.aX = e.jaxis.value / 256;
          if (e.jaxis.axis == 3) ctrl.aY = e.jaxis.value / 256;
        }
      }
      if (e.type == SDL_JOYBUTTONDOWN) {
        if (e.jbutton.which == ctrl.id) {
          ctrl.b |= (1 << (e.jbutton.button)) & ((1 << 8) - 1);
          if (e.jbutton.button == 8) ctrl.home = 1;
        }
      }
      if (e.type == SDL_JOYBUTTONUP) {
        if (e.jbutton.which == ctrl.id) {
          ctrl.b &= ~((1 << e.cbutton.button) & ((1 << 8) - 1));
          if (e.jbutton.button == 8) ctrl.home = 0;
        }
      }
    }
  }*/
  if (e.type == SDL_JOYDEVICEREMOVED) {
    if (e.jdevice.which == ctrl.id) {
      SDL_JoystickClose(ctrl.jCtrl);
      ctrl.jCtrl = nullptr;
      SDL_GameControllerClose(ctrl.cCtrl);
      ctrl.cCtrl = nullptr;
      /*SDL_HapticClose(ctrl.rumble);
      ctrl.rumble = nullptr;*/
      if (ctrl.id == 0) ctrl.type = CTRL_TYPE_KEYBOARD;
      else ctrl.id = -1;
      SDL_Log("Joystick %i disconnected\n", e.jdevice.which);
      return 1;
    }
  }
  return 0;
}

bool getKey(SDL_Keycode key) {
  return SDL_GetKeyboardState(NULL)[key];
}

void ctrlUpdate(CtrlList &ctrls) {
  controllerTypes &= 0;
  for (int i = 0; i < ctrls.ctrlCount; i++) {
    ctrls.ctrls[i]->b = 0;
    ctrls.ctrls[i]->aX = 0;
    ctrls.ctrls[i]->aY = 0;
    if (!ctrls.ctrls[i]) continue;
    if (ctrls.ctrls[i]->id == -1 && i != 0) {
      delete ctrls.ctrls[i];
      ctrls.ctrls[i] = nullptr;
      ctrls.ctrlCount--;
      continue;
    }
    if (ctrls.ctrls[i]->cCtrl && !SDL_GameControllerGetAttached(ctrls.ctrls[i]->cCtrl)) {
      SDL_JoystickClose(ctrls.ctrls[i]->jCtrl);
      ctrls.ctrls[i]->jCtrl = nullptr;
      SDL_GameControllerClose(ctrls.ctrls[i]->cCtrl);
      ctrls.ctrls[i]->cCtrl = nullptr;
      if (ctrls.ctrls[i]->id == 0) ctrls.ctrls[i]->type = CTRL_TYPE_KEYBOARD;
      else ctrls.ctrls[i]->id = -1;
      continue;
    }
    if (ctrls.ctrls[i]->type == CTRL_TYPE_KEYBOARD) {
      if (ctrls.ctrls[i]->id != 0) {
        ctrls.ctrls[i]->id = -1;
        continue;
      }
      if (getKey(keyJump)) ctrls.ctrls[i]->b |= CTRL_ACTION_GAME_JUMP;
      if (getKey(keyDash)) ctrls.ctrls[i]->b |= CTRL_ACTION_GAME_RUN;
      if (getKey(keyPause)) ctrls.ctrls[i]->b |= CTRL_ACTION_GAME_PAUSE;
      if (getKey(keyBack)) ctrls.ctrls[i]->b |= CTRL_ACTION_GAME_SQUISH;
      if (getKey(keySubMenu)) ctrls.ctrls[i]->b |= CTRL_ACTION_MENU_SUBMENU;
      if (getKey(keySpecial)) ctrls.ctrls[i]->b |= CTRL_ACTION_MENU_SPECIAL;
      if (!(getKey(keyUp) && getKey(keyDown))) {
        if (getKey(keyUp)) ctrls.ctrls[i]->aY = -127;
        if (getKey(keyDown)) ctrls.ctrls[i]->aY = 127;
      }
      if (!(getKey(keyLeft) && getKey(keyRight))) {
        if (getKey(keyLeft)) ctrls.ctrls[i]->aX = -127;
        if (getKey(keyRight)) ctrls.ctrls[i]->aX = 127;
      }
    }
    if (ctrls.ctrls[i]->type == CTRL_TYPE_XINPUT) {
      SDL_GameControllerUpdate();
      if (!ctrls.ctrls[i]->cCtrl) continue;
      /*if (SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_A)) ctrls.ctrls[i]->b |= CTRL_ACTION_GAME_JUMP;
      else ctrls.ctrls[i]->b &= ~CTRL_ACTION_GAME_JUMP;
      if (SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_X)) ctrls.ctrls[i]->b |= CTRL_ACTION_GAME_RUN;
      else ctrls.ctrls[i]->b &= ~CTRL_ACTION_GAME_RUN;
      if (SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_B)) ctrls.ctrls[i]->b |= CTRL_ACTION_GAME_SQUISH;
      else ctrls.ctrls[i]->b &= ~CTRL_ACTION_GAME_SQUISH;
      if (SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_Y)) ctrls.ctrls[i]->b |= CTRL_ACTION_MENU_SPECIAL;
      else ctrls.ctrls[i]->b &= ~CTRL_ACTION_MENU_SPECIAL;
      if (SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_START)) ctrls.ctrls[i]->b |= CTRL_ACTION_GAME_PAUSE;
      else ctrls.ctrls[i]->b &= ~CTRL_ACTION_GAME_PAUSE;
      if (SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_BACK)) ctrls.ctrls[i]->b |= CTRL_ACTION_MENU_SUBMENU;
      else ctrls.ctrls[i]->b &= ~CTRL_ACTION_MENU_SUBMENU;*/
      ctrls.ctrls[i]->b |= CTRL_ACTION_GAME_JUMP * SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_A);
      ctrls.ctrls[i]->b |= CTRL_ACTION_GAME_RUN * SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_X);
      ctrls.ctrls[i]->b |= CTRL_ACTION_GAME_SQUISH * SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_B);
      ctrls.ctrls[i]->b |= CTRL_ACTION_MENU_SPECIAL * SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_Y);
      ctrls.ctrls[i]->b |= CTRL_ACTION_GAME_PAUSE * SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_START);
      ctrls.ctrls[i]->b |= CTRL_ACTION_MENU_SUBMENU * SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_BACK);
      if (SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) ctrls.ctrls[i]->aX = -127;
      else if (SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) ctrls.ctrls[i]->aX = 127;
      else ctrls.ctrls[i]->aX = SDL_GameControllerGetAxis(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_AXIS_LEFTX) / 256;
      if (SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_DPAD_UP)) ctrls.ctrls[i]->aY = -127;
      else if (SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) ctrls.ctrls[i]->aY = 127;
      else ctrls.ctrls[i]->aY = SDL_GameControllerGetAxis(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_AXIS_LEFTY) / 256;
      ctrls.ctrls[i]->a2X = SDL_GameControllerGetAxis(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_AXIS_RIGHTX) / 256;
      ctrls.ctrls[i]->a2Y = SDL_GameControllerGetAxis(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_AXIS_RIGHTY) / 256;
      ctrls.ctrls[i]->lastScreenshot = ctrls.ctrls[i]->screenshot;
      ctrls.ctrls[i]->screenshot = SDL_GameControllerGetButton(ctrls.ctrls[i]->cCtrl, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    }
    controllerTypes |= ctrls.ctrls[i]->type;
    ctrls.ctrls[i]->lastType = ctrls.ctrls[i]->type;
  }
  //if (!controllerTypes) controllerTypes = CTRL_TYPE_KEYBOARD;
}

void ctrlCheckForNew(SDL_Event e, CtrlList &ctrls) {
  if (e.type == SDL_JOYDEVICEADDED) {
    if (ctrls.ctrlCount > 7) return;
    Controller *ctrl = new Controller;
    ctrl->jCtrl = SDL_JoystickOpen(e.jdevice.which);
    ctrl->id = e.jdevice.which;
    if (SDL_IsGameController(ctrl->id)) {
      ctrl->type = CTRL_TYPE_XINPUT;
      ctrl->lastType = ctrl->type;
      ctrl->cCtrl = SDL_GameControllerOpen(e.jdevice.which);
      /*ctrl->rumble = SDL_HapticOpen(ctrl->id);
      if (ctrl->rumble) {
        if (SDL_HapticRumbleInit(ctrl->rumble)) {
          SDL_HapticClose(ctrl->rumble);
          ctrl->rumble = nullptr;
        } else if (vibrate) {
          SDL_HapticRumblePlay(ctrl->rumble, 1.0f, 1000);
        }
      }*/
    } else {
      if (ctrl->id > 0) ctrl->id = -1;
      else ctrl->type = CTRL_TYPE_KEYBOARD;
      ctrl->lastType = ctrl->type;
      SDL_JoystickClose(ctrl->jCtrl);
      ctrl->jCtrl = nullptr;
      return;
    }
    ctrl->aX = 0, ctrl->aY = 0, ctrl->b = 0;
    if (ctrls.ctrls[0]->type == CTRL_TYPE_KEYBOARD) {
      delete ctrls.ctrls[0];
      ctrls.ctrls[0] = ctrl;
    } else {
      ctrls.ctrls[ctrls.ctrlCount] = ctrl;
      ctrls.ctrlCount++;
    }
    SDL_Log("Controller %i connected. Is type %i and is player %i\n", e.jdevice.which, ctrl->type, ctrl->id);
  }
}

CtrlList ctrlInit(bool useKeyboard, bool rumble) {
  keys = useKeyboard;
  vibrate = rumble;
  ctrlKeyboard = IMG_Load(pathForData("imgs/controls/keyboard.png"));
  ctrlXinput = IMG_Load(pathForData("imgs/controls/xinput.png"));
  CtrlList ctrls;
  ctrls.ctrlCount = SDL_NumJoysticks();
  if (ctrls.ctrlCount < 1) ctrls.ctrlCount = 1;
  if (ctrls.ctrlCount > 8) ctrls.ctrlCount = 8;
  for (int i = 0; i < 8; i++) {
    Controller *ctrl = new Controller;
    memset(ctrl, 0, sizeof(ctrl));
    ctrl->id = -1;
    ctrl->jCtrl = nullptr;
    ctrl->cCtrl = nullptr;
    ctrl->rumble = nullptr;
    ctrl->type = CTRL_TYPE_KEYBOARD;
    ctrl->lastType = ctrl->type;
    if (i <= ctrls.ctrlCount) {
      ctrl->jCtrl = SDL_JoystickOpen(i);
      if (ctrl->jCtrl) {
        ctrl->id = i;
        if (SDL_IsGameController(i)) {
          ctrl->type = CTRL_TYPE_XINPUT;
          ctrl->cCtrl = SDL_GameControllerOpen(i);
          /*ctrl->rumble = SDL_HapticOpen(i);
          if (ctrl->rumble) {
            if (SDL_HapticRumbleInit(ctrl->rumble)) {
              SDL_HapticClose(ctrl->rumble);
              ctrl->rumble = nullptr;
            }
          }*/
        } else {
          SDL_Log("Joystick is not controller");
          if (i > 0) ctrl->id = -1;
          else ctrl->type = CTRL_TYPE_KEYBOARD;
          SDL_JoystickClose(ctrl->jCtrl);
          ctrl->jCtrl = nullptr;
        }
      } else {
        if (i > 0) ctrl->id = -1;
        else ctrl->type = CTRL_TYPE_KEYBOARD;
        SDL_JoystickClose(ctrl->jCtrl);
        ctrl->jCtrl = nullptr;
      }
    } else {
      ctrl->id = 0;
      ctrl->type = CTRL_TYPE_KEYBOARD;
    }
    ctrl->aX = 0, ctrl->aY = 0, ctrl->b = 0;
    if (ctrl->type == CTRL_TYPE_KEYBOARD) ctrl->id = 0;
    ctrls.ctrls[i] = ctrl;
  }
  SDL_GameControllerEventState(1);
  return ctrls;
}

void ctrlRumble(Controller &ctrl, float power, int time) {
  if (!vibrate || ctrl.type == CTRL_TYPE_KEYBOARD) return;
  if (power < 0.0f) power = 0.0f;
  if (power > 1.0f) power = 1.0f;
  //if (ctrl.rumble) SDL_HapticRumblePlay(ctrl.rumble, power, time);
  SDL_GameControllerRumble(ctrl.cCtrl, float(0xffff) * power, float(0xffff) * power, time);
}

void ctrlSetRumble(bool active) {
  vibrate = active;
}

void ctrlClose(CtrlList &ctrls) {
  for (int i = 0; i < ctrls.ctrlCount; i++) {
    if (ctrls.ctrls[i]->jCtrl) {
      SDL_JoystickClose(ctrls.ctrls[i]->jCtrl);
      ctrls.ctrls[i]->jCtrl = nullptr;
      SDL_GameControllerClose(ctrls.ctrls[i]->cCtrl);
      ctrls.ctrls[i]->cCtrl = nullptr;
      /*SDL_HapticClose(ctrls.ctrls[i]->rumble);
      ctrls.ctrls[i]->rumble = nullptr;*/
      delete ctrls.ctrls[i];
    }
  }
  SDL_FreeSurface(ctrlKeyboard);
  SDL_FreeSurface(ctrlXinput);
}

char ctrlActionToChr(int actionName) {
  if (actionName == CTRL_ACTIONNAME_MENU_MOVE) return 'L';
  if (actionName == CTRL_ACTIONNAME_MENU_OK) return 'K';
  if (actionName == CTRL_ACTIONNAME_MENU_BACK) return 'B';
  if (actionName == CTRL_ACTIONNAME_MENU_SPECIAL) return 'Y';
  if (actionName == CTRL_ACTIONNAME_MENU_SUBMENU) return '<';

  if (actionName == CTRL_ACTIONNAME_GAME_MOVE) return 'L';
  if (actionName == CTRL_ACTIONNAME_GAME_LOOK) return 'R';
  if (actionName == CTRL_ACTIONNAME_GAME_JUMP) return 'A';
  if (actionName == CTRL_ACTIONNAME_GAME_RUN) return 'X';
  if (actionName == CTRL_ACTIONNAME_GAME_SQUISH) return 'B';
  if (actionName == CTRL_ACTIONNAME_GAME_PAUSE) return '>';

  if (actionName == CTRL_ACTIONNAME_SCREENSHOT) return 'P';
  return '?';
}

char *getTextForButton(char action, int *charCount) {
  static char text[64];
  memset(text, 0, 64);
  for (int i = 0; i < 8; i++) {
    if (controllerTypes & (1 << i)) {
      switch (i) {
        case CTRL_TYPE_KEYBOARD:
          if (action == 'L') strcat(text, "WASD");
          if (action == 'A') strcat(text, "_");
          if (action == 'X') strcat(text, "^");
          if (action == 'K') strcat(text, "_>");
          break;
        case CTRL_TYPE_XINPUT:
          if (action == 'L') strcat(text, "L");
          if (action == 'R') strcat(text, "R");
          if (action == 'A') strcat(text, "a");
          if (action == 'B') strcat(text, "b");
          if (action == 'X') strcat(text, "x");
          if (action == 'Y') strcat(text, "y");
          if (action == '<') strcat(text, "<");
          if (action == '>') strcat(text, ">");
          if (action == 'K') strcat(text, "a>");
          break;
      }
    }
  }
  if (strlen(text) < 1) text[0] = '?';
  *charCount = strlen(text);
  return text;
}

SDL_Surface *ctrlGetGlyphForButton(char action, int color) {
  SDL_Surface *glyph = SDL_CreateRGBSurfaceWithFormat(0, 16, 16, 32, SDL_PIXELFORMAT_RGBA32);
  SDL_Surface *glyphs[4];
  for (int i = 0; i < 4; i++) glyphs[i] = SDL_CreateRGBSurfaceWithFormat(0, 16, 16, 32, SDL_PIXELFORMAT_RGBA32);
  int glyphCount = 0;
  SDL_Rect clip = {0, 0, 16, 16};
  /*for (int i = 0; i < 8; i++) {
    if (controllerTypes & (1 << i)) {
      clip.x = -1;
      switch (i) {
        case CTRL_TYPE_KEYBOARD:
          //if (!ctrlKeyboard) goto noImg;
          if (action == 'L') clip.x = 0;
          if (action == 'A' || action == 'K') clip.x = 16;
          if (action == 'X') clip.x = 32;
          if (clip.x > -1) {
            SDL_BlitSurface(ctrlKeyboard, &clip, glyphs[glyphCount], NULL);
            glyphCount++;
          }
          break;
        case CTRL_TYPE_XINPUT:
          //if (!ctrlXinput) goto noImg;
          if (action == 'L') {
            clip.x = 0;
            SDL_BlitSurface(ctrlXinput, &clip, glyphs[glyphCount], NULL);
            glyphCount++;
            clip.x = 224;
            SDL_BlitSurface(ctrlXinput, &clip, glyphs[glyphCount], NULL);
            glyphCount++;
            break;
          }
          if (action == 'R') clip.x = 16;
          if (action == 'A') clip.x = 32;
          if (action == 'B') clip.x = 48;
          if (action == 'X') clip.x = 64;
          if (action == 'Y') clip.x = 80;
          if (action == '<') clip.x = 96;
          if (action == '>') clip.x = 112;
          if (action == 'K') {
            clip.x = 32;
            SDL_BlitSurface(ctrlXinput, &clip, glyphs[glyphCount], NULL);
            glyphCount++;
            clip.x = 112;
            SDL_BlitSurface(ctrlXinput, &clip, glyphs[glyphCount], NULL);
            glyphCount++;
            break;
          }
          if (clip.x > -1) {
            SDL_BlitSurface(ctrlXinput, &clip, glyphs[glyphCount], NULL);
            glyphCount++;
          }
          break;
      }
    }
  }*/
  clip.x = -1;
  if (controllerTypes & CTRL_TYPE_KEYBOARD) {
    if (action == 'L') clip.x = 0;
    if (action == 'A' || action == 'K') clip.x = 16;
    if (action == 'X') clip.x = 32;
    if (action == 'B') clip.x = 48;
    if (action == '<') clip.x = 64;
    if (action == 'Y') clip.x = 80;
    if (clip.x > -1) {
      SDL_BlitSurface(ctrlKeyboard, &clip, glyphs[glyphCount], NULL);
      glyphCount++;
      clip.x = -1;
    }
  }
  if (controllerTypes & CTRL_TYPE_XINPUT) {
    if (action == 'L') {
      clip.x = 0;
      SDL_BlitSurface(ctrlXinput, &clip, glyphs[glyphCount], NULL);
      glyphCount++;
      clip.x = 224;
      SDL_BlitSurface(ctrlXinput, &clip, glyphs[glyphCount], NULL);
      glyphCount++;
      clip.x = -1;
    }
    if (action == 'R') clip.x = 16;
    if (action == 'A') clip.x = 32;
    if (action == 'B') clip.x = 48;
    if (action == 'X') clip.x = 64;
    if (action == 'Y') clip.x = 80;
    if (action == '<') clip.x = 96;
    if (action == '>') clip.x = 112;
    if (action == 'K') {
      clip.x = 32;
      SDL_BlitSurface(ctrlXinput, &clip, glyphs[glyphCount], NULL);
      glyphCount++;
      clip.x = 112;
      SDL_BlitSurface(ctrlXinput, &clip, glyphs[glyphCount], NULL);
      glyphCount++;
      clip.x = -1;
    }
    if (action == 'P') clip.x = 208;
    if (clip.x > -1) {
      SDL_BlitSurface(ctrlXinput, &clip, glyphs[glyphCount], NULL);
      glyphCount++;
    }
  }
  if (glyphCount != 0) SDL_BlitSurface(glyphs[(getTimer() / 60) % glyphCount], NULL, glyph, NULL);
  for (int i = 0; i < 4; i++) SDL_FreeSurface(glyphs[i]);
  return glyph;
  noImg:
  SDL_Surface *text = nullptr;
  int charCount = 0;
  text = surfaceText(getTextForButton(action, &charCount), 1, 0, color);
  int cR = 0, cG = 0, cB = 0;
  if (color == 1) cR = 0x08, cG = 0xa0;
  if (color == 2) cR = 0x20, cG = 0xf0;
  if (color == 3) cR = 0xa0, cG = 0xff, cB = 0x40;
  SDL_Rect rect = {0, 0, 16, 16};
  SDL_FillRect(glyph, &rect, SDL_MapRGB(glyph->format, cR, cG, cB));
  cR = 0, cG = 0, cB = 0;
  if (color - 1 == 1) cR = 0x08, cG = 0xa0;
  if (color - 1 == 2) cR = 0x20, cG = 0xf0;
  if (color == 0) cR = 0xa0, cG = 0xff, cB = 0x40;
  rect = {1, 1, 14, 14};
  SDL_FillRect(glyph, &rect, SDL_MapRGB(glyph->format, cR, cG, cB));
  rect.x = 8 - ((text->w / (charCount + 1)) / 2);
  clip = {(text->w / (charCount + 1)) * ((getTimer() / 60) % charCount), 0, (text->w / (charCount + 1)), text->h};
  SDL_BlitSurface(text, &clip, glyph, &rect);
  SDL_FreeSurface(text);
  return glyph;
}