#include "controller.h"
#include <SDL2/SDL.h>
#include "helpers.h"

auto keyUp = SDLK_w, keyDown = SDLK_s, keyLeft = SDLK_a, keyRight = SDLK_d, keyJump = SDLK_SPACE, keyDash  = SDLK_LSHIFT, keyBoost = SDLK_k, keyPause = SDLK_RETURN;
unsigned long lastRumble = 0;
unsigned short rumbleWait = 0;

bool ctrlUpdate(SDL_Event e, Controller &ctrl) {
  if (e.type == SDL_KEYDOWN) {
    if (e.key.keysym.sym == keyUp) ctrl.aY = -127;
    if (e.key.keysym.sym == keyDown) ctrl.aY = 127;
    if (e.key.keysym.sym == keyLeft) ctrl.aX = -127;
    if (e.key.keysym.sym == keyRight) ctrl.aX = 127;
    if (e.key.keysym.sym == keyJump) ctrl.b |= 0b1;
    if (e.key.keysym.sym == keyDash) ctrl.b |= 0b100;
    if (e.key.keysym.sym == keyBoost) ctrl.b |= 0b10000000;
    if (e.key.keysym.sym == keyPause) ctrl.b |= 0b100000;
  }
  if (e.type == SDL_KEYUP) {
    if (e.key.keysym.sym == keyUp && ctrl.aY == -127) ctrl.aY = 0;
    if (e.key.keysym.sym == keyDown && ctrl.aY == 127) ctrl.aY = 0;
    if (e.key.keysym.sym == keyLeft && ctrl.aX == -127) ctrl.aX = 0;
    if (e.key.keysym.sym == keyRight && ctrl.aX == 127) ctrl.aX = 0;
    if (e.key.keysym.sym == keyJump) ctrl.b &= ~0b1;
    if (e.key.keysym.sym == keyDash) ctrl.b &= ~0b100;
    if (e.key.keysym.sym == keyBoost) ctrl.b &= ~0b10000000;
    if (e.key.keysym.sym == keyPause) ctrl.b &= ~0b100000;
  }
  if (ctrl.jCtrl) {
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
          if (e.cbutton.button == 6) ctrl.home = 1;
        }
      }
      if (e.type == SDL_CONTROLLERBUTTONUP) {
        if (e.cbutton.which == ctrl.id) {
          ctrl.b &= ~((1 << (e.cbutton.button - (e.cbutton.button > 4) - 2 * (e.cbutton.button > 6)) & ((1 << 8) - 1)));
          if (e.cbutton.button == 6) ctrl.home = 0;
        }
      }
    } else {
      if (e.type == SDL_JOYAXISMOTION) {
        if (e.jaxis.which == ctrl.id) {
          if (e.jaxis.axis == 0) ctrl.aX = e.jaxis.value / 256;
          if (e.jaxis.axis == 1) ctrl.aY = e.jaxis.value / 256;
          if (e.jaxis.axis == 2) ctrl.a2X = e.jaxis.value / 256;
          if (e.jaxis.axis == 2) ctrl.a2Y = e.jaxis.value / 256;
        }
      }
      if (e.type == SDL_JOYBUTTONDOWN) {
        if (e.jbutton.which == ctrl.id) {
          ctrl.b |= (1 << (e.jbutton.button) & ((1 << 8) - 1));
        }
      }
      if (e.type == SDL_JOYBUTTONUP) {
        if (e.jbutton.which == ctrl.id) {
          ctrl.b &= ~(1 << (e.jbutton.button) & ((1 << 8) - 1));
        }
      }
    }
  }
  if (e.type == SDL_JOYDEVICEADDED) {
    if (!ctrl.jCtrl) {
      memset(&ctrl, 0, sizeof(&ctrl));
      ctrl.jCtrl = SDL_JoystickOpen(e.jdevice.which);
      ctrl.id = e.jdevice.which;
      if (SDL_IsGameController(ctrl.id)) ctrl.cCtrl = SDL_GameControllerOpen(e.jdevice.which);
      ctrl.rumble = SDL_HapticOpen(ctrl.id);
      if (ctrl.rumble) {
        if (SDL_HapticRumbleInit(ctrl.rumble)) {
          SDL_HapticClose(ctrl.rumble);
          ctrl.rumble = nullptr;
        }
      }
    }
  }
  if (e.type == SDL_JOYDEVICEREMOVED) {
    if (e.jdevice.which == ctrl.id) {
      SDL_JoystickClose(ctrl.jCtrl);
      ctrl.jCtrl = nullptr;
      SDL_GameControllerClose(ctrl.cCtrl);
      ctrl.cCtrl = nullptr;
      ctrl.id = -1;
      SDL_HapticClose(ctrl.rumble);
      ctrl.rumble = nullptr;
    }
    return 1;
  }
  return 0;
}

Controller ctrlInit() {
  SDL_JoystickEventState(1);
  SDL_GameControllerEventState(1);
  Controller ctrl;
  memset(&ctrl, 0, sizeof(&ctrl));
  ctrl.jCtrl = nullptr;
  ctrl.cCtrl = nullptr;
  ctrl.rumble = nullptr;
  SDL_Log("There are %i joysticks.", SDL_NumJoysticks());
  if (SDL_NumJoysticks() > 1) ctrl.jCtrl = SDL_JoystickOpen(0);
  if (ctrl.jCtrl) {
    ctrl.id = 0;
    if (SDL_IsGameController(0)) ctrl.cCtrl = SDL_GameControllerOpen(0);
    ctrl.rumble = SDL_HapticOpen(0);
    if (ctrl.rumble) {
      if (SDL_HapticRumbleInit(ctrl.rumble)) {
        SDL_HapticClose(ctrl.rumble);
        ctrl.rumble = nullptr;
      }
    }
    SDL_Log("Joystick '%s' connected.", SDL_JoystickName(ctrl.jCtrl));
    if (ctrl.cCtrl) SDL_Log("Joystick is controller. '%s'", SDL_GameControllerName(ctrl.cCtrl));
    if (ctrl.rumble) SDL_Log("Joystick supports rumble.");
  }
  return ctrl;
}

void ctrlRumble(Controller ctrl, float power, unsigned short time) {
  if (power < 0.5) return;
  if (ctrl.rumble && getTimer() - lastRumble > rumbleWait) {
    if (power > 1) power = 1;
    if (power < 0) power = 0;
    SDL_HapticRumblePlay(ctrl.rumble, float(power), time);
    lastRumble = getTimer();
    rumbleWait = time * 0.06;
  }
}

void ctrlClose(Controller ctrl) {
  if (ctrl.jCtrl) {
    SDL_JoystickClose(ctrl.jCtrl);
    ctrl.jCtrl = nullptr;
    SDL_GameControllerClose(ctrl.cCtrl);
    ctrl.cCtrl = nullptr;
    SDL_HapticClose(ctrl.rumble);
    ctrl.rumble = nullptr;
  }
}