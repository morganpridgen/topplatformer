#include <exception>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <nlohmann/json.hpp>
#include "window.h"
#include "helpers.h"
#include "player.h"
#include "controller.h"
#include "terrain.h"
#include "enemy.h"
#include "state.h"
#define SLVLSELECT 0
#define SGAME 1
#define SINTRO 2
#define SWIN 3
#define SOPTIONS 4
#define killWithHome 0
using namespace std::chrono;
using namespace std;
using json = nlohmann::json;

SDL_Event e;
CtrlList ctrls;
Window win;
Player ply;
Particle *particles[particleCount];
Enemy *enemies[enemyCount];
BadFish *badFish[badFishCount];
Shark *sharks[sharkCount];
char levelName[64];
Mix_Chunk *menuMus = nullptr;
Mix_Chunk *gameMus = nullptr;
bool running = 1;
int lListLen = 0;
json levelList;
bool pause = 0;
bool lastPause = 0;
bool lowDetail = 0;
bool useThreads = SDL_GetCPUCount() > 1;
bool fullscreen = 0;
bool vsync = 0;
bool useKeyboard = 0;
bool enableParticles = 1;
bool rumble = 1;
bool heavyRumble = 0;
bool simpleEffects = 0;
bool hideGui = 0;
bool music = 0;
bool lowRender = 0;
bool skipIntro = 0;
bool xfwmFull = 0;
bool fps30 = 0;
char name[] = "Top Platformer";
char me[] = "CurlyMorgan";
unsigned long lastFullToggle = 0;
LevelScores scores;
int lockedLevel = 1;
int sX = 0, sY = 0;

volatile GameState *state;

void loop();

void doOption(char option, bool active) {
  switch (option) {
    case 'f':
      fullscreen = active;
      break;
    case 'g':
      hideGui = active;
      break;
    case 'h':
      heavyRumble = active;
      break;
    case 'i':
      skipIntro = active;
      break;
    case 'k':
      useKeyboard = active;
      break;
    case 'l':
      lowRender = active;
      break;
    case 'm':
      music = active;
      break;
    case 'p':
      enableParticles = active;
      break;
    case 'r':
      rumble = active;
      break;
    case 's':
      simpleEffects = active;
      break;
    case 't':
      useThreads = active;
      break;
    case 'v':
      vsync = active;
      break;
  }
}

char chrOpt(bool option) {
  return option ? 'y' : 'n';
}

char *saveOptions() {
  /*data << 'f' << chrOpt(fullscreen);
  data << 'g' << chrOpt(hideGui);
  data << 'h' << chrOpt(heavyRumble);
  data << 'i' << chrOpt(skipIntro);
  data << 'k' << chrOpt(useKeyboard);
  data << 'l' << chrOpt(lowRender);
  data << 'm' << chrOpt(music);
  data << 'p' << chrOpt(enableParticles);
  data << 'r' << chrOpt(rumble);
  data << 's' << chrOpt(simpleEffects);
  data << 't' << chrOpt(useThreads);
  data << 'v' << chrOpt(vsync);*/
  char *buffer = new char[2 * 12 + 1];
  memset(buffer, 0, sizeof(buffer));
  sprintf(buffer, "f%cg%ch%ci%ck%cl%cm%cp%cr%cs%ct%cv%c", chrOpt(fullscreen), chrOpt(hideGui), chrOpt(heavyRumble), chrOpt(skipIntro), chrOpt(useKeyboard), chrOpt(lowRender), chrOpt(music), chrOpt(enableParticles), chrOpt(rumble), chrOpt(simpleEffects), chrOpt(useThreads), chrOpt(vsync));
  return buffer;
}

bool init() {
  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
  SDL_SetHint(SDL_HINT_RENDER_LOGICAL_SIZE_MODE, "letterbox");
  SDL_SetHint(SDL_HINT_RENDER_OPENGL_SHADERS, "0");
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
    SDL_Log("[ERROR] SDL didn't initiate. %s\n", SDL_GetError());
    return 0;
  }
  SDL_DisplayMode mode;
  SDL_GetCurrentDisplayMode(0, &mode);
  bool winActive = 0;
  if (fullscreen && !xfwmFull) {
    sX = mode.w;
    sY = mode.h;
    winActive = win.makeScreen(sX, sY, name, 0, vsync, lowRender);
  } else {
    sX = gamemax(mode.w / 3 * 2, intResX);
    sY = gamemax(mode.h / 3 * 2, intResY);
    winActive = win.makeScreen(sX, sY, name, 1, vsync, lowRender);
  }
  if (!winActive) {
    SDL_Log("[ERROR] Could not make a window. %s\n", SDL_GetError());
    return 0;
  }
  int imgFlags = IMG_INIT_PNG;
  if (!(IMG_Init(imgFlags) & imgFlags)) {
    SDL_Log("[ERROR] Couldn't set up SDL image. %s\n", SDL_GetError());
    return 0;
  }
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 1, 1 << 12) == -1) {
    SDL_Log("[ERROR] Couldn't set up SDL mixer. %s\n", Mix_GetError());
    return 0;
  }
  FILE *file = fopen(pathForData("levels/levellist.json"), "r");
  if (file) {
    fseek(file, 0, SEEK_END);
    int length = ftell(file) + 1L;
    fseek(file, 0, SEEK_SET);
    char *buffer = new char[length];
    memset(buffer, 0, length);
    fread(buffer, length, 1, file);
    fclose(file);
    levelList = json::parse(buffer);
    delete [] buffer;
  } else {
    SDL_Log("[ERROR] Couldn't get list of levels.\n");
    return 0;
  }
  memset(levelName, 0, 64);
  strcpy(levelName, levelList[0].get<string>().c_str());
  lListLen = levelList.size();
  file = fopen(pathForSaves("complete"), "rb");
  if (file) {
    fread((char *)&lockedLevel, sizeof(lockedLevel), 1, file);
    fclose(file);
  }
  ctrls = ctrlInit(useKeyboard, rumble);
  if (!ply.init(win, heavyRumble)) return 0;
  if (music) {
    menuMus = Mix_LoadWAV(pathForData("music/menu.wav"));
    if (!menuMus) {
      SDL_Log("[ERROR] Can't load music. %s\n", Mix_GetError());
    } else Mix_VolumeChunk(menuMus, (MIX_MAX_VOLUME / 4) * 3);
    gameMus = Mix_LoadWAV(pathForData("music/level.wav"));
    if (!gameMus) {
      SDL_Log("[ERROR] Can't load music. %s\n", Mix_GetError());
    } else Mix_VolumeChunk(gameMus, (MIX_MAX_VOLUME / 4) * 3);
  }
  if (!loadFonts()) return 0;
  if (!enemyLoadRes(win)) return 0;
  if (!terrainLoadRes(win)) return 0;
  memset(&scores, 0, sizeof(scores));
  
  state = new IntroState;
  if (!state->init()) return 0;
  return 1;
}

void event(SDL_Event *event) {
  SDL_Event e = *event;
  switch (e.type) {
    case SDL_QUIT:
      running = 0;
      break;
    case SDL_KEYDOWN:
      switch (e.key.keysym.sym) {
        case SDLK_F11:
          if (getTimer() - lastFullToggle > 60) {
            lastFullToggle = getTimer();
            fullscreen = !fullscreen;
            SDL_DisplayMode mode;
            SDL_GetCurrentDisplayMode(0, &mode);
            if (fullscreen) {
              sX = mode.w;
              sY = mode.h;
            } else {
              sX = gamemax(mode.w / 3, intResX);
              sY = gamemax(mode.h / 3, intResY);
            }
            win.resizeScreen(sX, sY, fullscreen);
          }
          break;
      }
      break;
    case SDL_WINDOWEVENT:
      if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
        sX = e.window.data1;
        sY = e.window.data2;
        win.resizeScreen(sX, sY, 0);
      }
      if (e.window.event == SDL_WINDOWEVENT_HIDDEN || e.window.event == SDL_WINDOWEVENT_MINIMIZED || e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) pause = 1;
      break;
  }
  for (int i = 0; i < 8; i++) {
    if (ctrls.ctrls[i]) {
      if (ctrlEvent(e, *ctrls.ctrls[i])) pause = 1;
    }
  }
  ctrlCheckForNew(e, ctrls);
}

void loop() {
  ctrlUpdate(ctrls);
  updateTimer();
  #if killWithHome == 1
    if (ctrls.ctrls[0]->home) running = 0;
  #endif
  volatile GameState *newState = state->update();
  if (newState) {
    if (state) delete state;
    state = newState;
    if (state != newState) running = 0;
    if (!state->init()) running = 0;
  }
}

void render() {
  state->render();
  win.update();
}

void end() {
  delete state;
  for (int i = 0; i < particleCount; i++) {
    if (particles[i]) delete particles[i];
  }
  for (int i = 0; i < enemyCount; i++) {
    if (enemies[i]) delete enemies[i];
  }
  unloadFonts();
  unloadTerrain();
  enemyFreeRes();
  ply.end();
  win.end();
  ctrlClose(ctrls);
  Mix_FreeChunk(menuMus);
  Mix_FreeChunk(gameMus);
  //freeAmbience();
  Mix_CloseAudio();
  SDL_Quit();
  FILE *file;
  file = fopen(pathForSaves("settings.cfg"), "w");
  if (file) {
    char *buffer = saveOptions();
    fwrite(buffer, strlen(buffer), 1, file);
    delete [] buffer;
    fclose(file);
  }
  file = fopen(pathForSaves("complete"), "wb");
  if (file) {
    fwrite(&lockedLevel, sizeof(lockedLevel), 1, file);
    fclose(file);
  }
}

int main(int argc, char** argv) {
  initPaths();
  #ifdef __linux__
    if (system("pgrep -l xfwm > /dev/null") == 0) xfwmFull = 1;
  #endif
  SDL_version compiled;
  SDL_version linked;
  SDL_VERSION(&compiled);
  SDL_GetVersion(&linked);
  SDL_Log("Compiling with SDL %d.%d.%d. Linking with SDL %d.%d.%d", compiled.major, compiled.minor, compiled.patch, linked.major, linked.minor, linked.patch);
  FILE *file;
  file = fopen(pathForSaves("settings.cfg"), "r");
  if (file) {
    fseek(file, 0, SEEK_END);
    int length = ftell(file) + 1L;
    fseek(file, 0, SEEK_SET);
    if (length % 2 == 1) {
      for (int i = 0; i < length / 2; i++) {
        char option = 0;
        char active = 0;
        fread(&option, 1, 1, file);
        fread(&active, 1, 1, file);
        //SDL_Log("Option %c is '%c'", option, active);
        doOption(option, active == 'y');
      }
    } else SDL_Log("Warning! Options file is in incorrect format.");
    fclose(file);
  }
  for (int i = i; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (strlen(argv[i]) > 2) {
        doOption(argv[i][1], argv[i][2] == 'y');
      } else {
        doOption(argv[i][1], 1);
      }
    }
  }
  auto lastTime = high_resolution_clock::now();
  auto lastLoop = lastTime;
  int loopCount = 1;
  unsigned int avgFrame = 16667, minFrame = -1, maxFrame = 0;
  unsigned long long totalFrame = 0;
  unsigned long frameCount = 0;
  try {
    SDL_Log("Starting.\n");
    if (init()) {
      while (running) {
        while(SDL_PollEvent(&e)) {
          event(&e);
        }
        if (duration_cast<microseconds>(high_resolution_clock::now() - lastLoop).count() > 15000) {
          for (int i = 0; i < loopCount; i++) loop();
          lastLoop = high_resolution_clock::now();
        }
        render();
        if (xfwmFull && fullscreen && frameCount == 0) {
          SDL_DisplayMode mode;
          SDL_GetCurrentDisplayMode(0, &mode);
          sX = mode.w;
          sY = mode.h;
          win.resizeScreen(sX, sY, 1);
        }
        while (fps30 && duration_cast<microseconds>(high_resolution_clock::now() - lastTime).count() < 1000000 / 30);
        auto doneTime = duration_cast<microseconds>(high_resolution_clock::now() - lastTime).count();
        lastTime = high_resolution_clock::now();
        loopCount = round(float(doneTime) / 16667.0);
        if (loopCount < 1) loopCount = 1;
        totalFrame += doneTime;
        avgFrame = totalFrame / (frameCount + 1);//(avgFrame + doneTime) / 2;
        if (doneTime < minFrame) minFrame = doneTime;
        if (doneTime > maxFrame) maxFrame = doneTime;
        frameCount++;
        /*if (doneTime - avgFrame > 6666 && avgFrame < 34333) lowDetail = 1;
        if (abs(doneTime - avgFrame) > 1667 && avgFrame > 33333) lowDetail = 0;
        win.info.lowDetail = lowDetail;*/
      }
      end();
      SDL_Log("Frames took an average of %i microseconds. The longest frame was %i, and the shortest was %i. Game was playing for %i frames.", avgFrame, maxFrame, minFrame, frameCount);
      SDL_Log("Sucess!\n");
      return 0;
    }
    SDL_Log("Error during initialization.\n");
    return 1;
  } catch (exception &ex) {
    SDL_Log("Crash! %s\n", ex.what());
    end();
    return 2;
  }
}
