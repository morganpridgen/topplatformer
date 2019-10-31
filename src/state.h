#ifndef stateh
#define stateh
#include <SDL2/SDL.h>

class GameState {
  private:
  public:
    virtual bool init() volatile = 0;
    virtual GameState *update() volatile = 0;
    virtual void threadFunc() volatile {}
    virtual void render() volatile = 0;
    virtual void end() volatile = 0;
};

class LevelSelectState : public GameState {
  public:
    virtual bool init() volatile;
    virtual GameState *update() volatile;
    virtual void render() volatile;
    virtual void end() volatile;
};

class PlayState : public GameState {
  public:
    virtual bool init() volatile;
    virtual GameState *update() volatile;
    virtual void threadFunc() volatile;
    virtual void render() volatile;
    virtual void end() volatile;
};

class IntroState : public GameState {
  public:
    virtual bool init() volatile;
    virtual GameState *update() volatile;
    virtual void render() volatile;
    virtual void end() volatile;
};

class WinState : public GameState {
  public:
    virtual bool init() volatile;
    virtual GameState *update() volatile;
    virtual void render() volatile;
    virtual void end() volatile;
};

class OptionsState : public GameState {
  public:
    virtual bool init() volatile;
    virtual GameState *update() volatile;
    virtual void render() volatile;
    virtual void end() volatile;
};

#endif