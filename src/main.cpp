#include <exception>
#include <chrono>
#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
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
//#include "ambience.h"
#define SLVLSELECT 0
#define SGAME 1
#define SINTRO 2
#define SWIN 3
#define SOPTIONS 4
#define killWithHome 0
using namespace std::chrono;
using namespace std;
using json = nlohmann::json;

CtrlList ctrls;
Window win;
Player ply;
Particle *particles[particleCount];
Enemy *enemies[enemyCount];
BadFish *badFish[badFishCount];
Shark *sharks[sharkCount];
Camera cam = {0, 0};
char levelName[64];
int screenNum = 0;
bool levelEnd;
SDL_Event e;
Mix_Chunk *menuMus = nullptr;
Mix_Chunk *gameMus = nullptr;
bool running = 1;
int state = SINTRO;
int lastState = -1;
bool stateChange = 1;
bool hasMoved = 0;
int selectedLevel = 0;
int lListLen = 0;
json levelList;
bool pause = 0;
bool lastPause = 0;
bool lowDetail = 0;
bool useThreads = SDL_GetCPUCount() > 1;
SDL_Thread *thread = nullptr;
int sX = 0;
int sY = 0;
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
bool superThread = 0;
bool fps30 = 0;
char name[] = "Top Platformer";
char me[] = "CurlyMorgan";
unsigned long lastFullToggle = 0;
SDL_Texture *stagePreview = nullptr;
SDL_Texture *unfinishedPreview = nullptr;
SDL_Texture *dispName = nullptr;
SDL_Texture *ctrlDisplay = nullptr;
SDL_Texture *starImg = nullptr;
unsigned long gameFrames = 0, screenFrames = 0, goalFrames1 = 60, goalFrames2 = 60, highScore = -1;
int introState = 0;
int introFrames = 0;
char introText[32] = "";
int levelFade = 1;
bool levelFadeDir = 1;
bool inWater = 0;
int deaths = 0;
bool assistMode = 0, assistModeFlip = 0, assistUsed = 0;
int grade = 0;
int option = 0;
bool optionAction = 0;
bool lastEffects = 0, lastGui = 0;
LevelScores scores;
int lockedLevel = 1;

SDL_Thread *mainThread = nullptr;
bool loopRunning = 1;
bool rendering = 0;

void loop();
int loopThread(void *data) {
  auto lastLoop = high_resolution_clock::now();
  while (loopRunning) {
    wait:
    if (duration_cast<microseconds>(high_resolution_clock::now() - lastLoop).count() < 1000000 / 60) goto wait;
    loop();
    lastLoop = high_resolution_clock::now();
  }
}

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

void saveOptions(fstream &data) {
  data << 'f' << chrOpt(fullscreen);
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
  data << 'v' << chrOpt(vsync);
}

int doThread(void *data) {
  switch (*static_cast<int *>(data)) {
    case SGAME:
      if (!pause) {
        Particle *tmpParts[particleCount];
        memcpy(tmpParts, particles, sizeof(particles));
        updateTerrain(ply, inWater, particles, enemies, badFish, sharks);
        if (enableParticles) {
          for (int i = 0; i < particleCount; i++) {
            if (particles[i]) {
              particles[i]->update();
              if (!particles[i]->getActive()) {
                delete particles[i];
                particles[i] = nullptr;
              }
            } else if (tmpParts[i]) {
              particles[i] = tmpParts[i];
            }
          }
        }
      }
      break;
  }
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
  fstream data;
  data.open(pathForData("levels/levellist.json"), ios::in);
  if (data) {
    data.seekg(0, data.end);
    int length = data.tellg() + 1L;
    data.seekg(0, data.beg);
    char *buffer = new char[length];
    memset(buffer, 0, length);
    data.read(buffer, length);
    data.close();
    levelList = json::parse(buffer);
  } else {
    SDL_Log("[ERROR] Couldn't get list of levels.\n");
    return 0;
  }
  memset(levelName, 0, 64);
  strcpy(levelName, levelList[0].get<string>().c_str());
  lListLen = levelList.size();
  data.open(pathForSaves("complete"), ios::in | ios::binary);
  if (data) data.read((char *)&lockedLevel, sizeof(lockedLevel));
  data.close();
  char previewPath[64];
  sprintf(previewPath, pathForData("screenshots/%s.png"), levelName);
  stagePreview = loadTexture(previewPath, win);
  if (!stagePreview) return 0;
  unfinishedPreview = loadTexture(pathForData("imgs/unfinished.png"), win);
  if (!unfinishedPreview) return 0;
  starImg = loadTexture(pathForData("imgs/star.png"), win);
  if (!starImg) return 0;
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
  //loadAmbience();
  if (!loadFonts()) return 0;
  if (!enemyLoadRes(win)) return 0;
  if (!terrainLoadRes(win)) return 0;
  memset(&scores, 0, sizeof(scores));
  if (superThread) mainThread = SDL_CreateThread(loopThread, "Test", NULL);
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
  SDL_WaitThread(thread, NULL);
  thread = nullptr;
  if (useThreads) thread = SDL_CreateThread(doThread, "Extras", (void *)&state);
  else doThread((void *)&state);
  switch (state) {
    char previewPath[64];
    case SLVLSELECT:
      if (nP(stateChange)) {
        //resetAmbience();
        Mix_HaltChannel(-1);
        if (music) Mix_PlayChannel(-1, menuMus, -1);
        for (int i = 0; i < particleCount; i++) {
          if (particles[i]) {
            delete particles[i];
            particles[i] = nullptr;
          }
        }
        for (int i = 0; i < enemyCount; i++) {
          if (enemies[i]) {
            delete enemies[i];
            enemies[i] = nullptr;
          }
        }
        for (int i = 0; i < badFishCount; i++) {
          if (badFish[i]) {
            delete badFish[i];
            badFish[i] = nullptr;
          }
        }
        for (int i = 0; i < sharkCount; i++) {
          if (sharks[i]) {
            delete sharks[i];
            sharks[i] = nullptr;
          }
        }
        scores = getScoresForLevel(levelName);
      }
      if (ctrls.ctrls[0]->b & CTRL_ACTION_MENU_SUBMENU) state = SOPTIONS;
      if (ctrls.ctrls[0]->b & CTRL_ACTION_MENU_OK && selectedLevel < lockedLevel) {
        SDL_WaitThread(thread, NULL);
        thread = nullptr;
        state = SGAME;
        screenNum = 0;
        PlayerInfo info = ply.getInfo();
        strcpy(levelName, levelList[selectedLevel].get<string>().c_str());
        StartPos start = loadLevel(levelName, screenNum);
        if (start.levelExists) {
          info.x = start.x;
          info.y = -8256;
          info.z = start.y;
          info.xVel = 0;
          info.yVel = 0;
          info.zVel = 0;
          info.flyAnim = 1;
          info.onGround = 1;
          info.faceAnim = 0;
          setSwitch(0, 1);
          ply.setInfo(info);
          inWater = 0;
          ply.updateCamera(cam, ctrls.ctrls[0], 1);
          pause = 0;
          lastPause = 1;
          gameFrames = 0;
          screenFrames = 0;
          levelFade = 1;
          levelFadeDir = 1;
          assistUsed = assistMode;
        } else state = SLVLSELECT;
      }
      if (!assistModeFlip && ctrls.ctrls[0]->b & CTRL_ACTION_MENU_SPECIAL) assistMode = !assistMode;
      assistModeFlip = ctrls.ctrls[0]->b & CTRL_ACTION_MENU_SPECIAL;
      if (abs(ctrls.ctrls[0]->aX) < 16) hasMoved = 0;
      if (!hasMoved) {
        if (ctrls.ctrls[0]->aX > 32) {
          hasMoved = 1;
          if (selectedLevel != lListLen - 1) selectedLevel++;
        }
        if (ctrls.ctrls[0]->aX < -32) {
          hasMoved = 1;
          if (selectedLevel != 0) selectedLevel--;
        }
        if (hasMoved) {
          SDL_DestroyTexture(stagePreview);
          strcpy(levelName, levelList[selectedLevel].get<string>().c_str());
          char previewPath[64];
          if (selectedLevel < lockedLevel) sprintf(previewPath, "screenshots/%s.png", levelName);
          else sprintf(previewPath, "screenshots/%s-locked.png", levelName);
          stagePreview = loadTexture(pathForData(previewPath), win);
          scores = getScoresForLevel(levelName);
        }
      }
      break;
    case SINTRO:
      if (skipIntro || ctrls.ctrls[0]->b & CTRL_ACTION_MENU_BACK) state = SLVLSELECT;
      if ((introState == 0 || introState == 2) && introFrames > 30) {
        introState ++;
        introFrames = 0;
      }
      if (introState == 1 && introFrames % 2 == 0) {
        for (int i = 0; i < 32; i++) {
          if (me[i] == 0) break;
          if (introText[i] == 0) {
            introText[i] = me[i];
            break;
          }
        }
        if (introFrames > 60) {
          introState = 2;
          introFrames = 0;
          memset(introText, 0, 32);
          break;
        }
      }
      if (introState == 3 && introFrames % 2 == 0) {
        for (int i = 0; i < 32; i++) {
          if (name[i] == 0) break;
          if (introText[i] == 0) {
            introText[i] = name[i];
            break;
          }
        }
        if (introFrames > 60) {
          introState = 4;
          introFrames = 0;
          break;
        }
      }
      if (introState == 4 && introFrames > 120) {
        introState = 5;
        introFrames = 0;
      }
      if (introState == 5 && introFrames > 60) state = SLVLSELECT;
      introFrames++;
      break;
    case SGAME: {
      if (nP(levelFade > 2 && !levelFadeDir && getTimer() % 4 == 0)) levelFade--;
      if (nP(levelFadeDir && getTimer() % 4 == 0)) {
        levelFade++;
        if (levelFade > 16) {
          levelFade = 0;
          levelFadeDir = 0;
        }
      }
      if (nP(stateChange)) {
        Mix_HaltChannel(-1);
        if (music) Mix_PlayChannel(-1, gameMus, -1);
        deaths = 0;
      }
      if (!lastPause && ctrls.ctrls[0]->b & CTRL_ACTION_GAME_PAUSE) pause = !pause;
      lastPause = ctrls.ctrls[0]->b & CTRL_ACTION_GAME_PAUSE;
      if (yP(!pause)) {
        gameFrames++;
        if (assistMode) assistUsed = 1;
        for (int i = 0; i < enemyCount; i++) {
          if (!enemies[i]) continue;
          enemies[i]->update();
          if (!enemies[i]->getActive()) {
            delete enemies[i];
            enemies[i] = nullptr;
          }
        }
        for (int i = 0; i < badFishCount; i++) {
          if (!badFish[i]) continue;
          badFish[i]->update(ply, particles);
        }
        for (int i = 0; i < sharkCount; i++) {
          if (!sharks[i]) continue;
          sharks[i]->update(ply, particles);
        }
        int playerCode = ply.update(ctrls.ctrls[0], assistMode, particles, enemies, badFish, sharks);
        if (nP(playerCode == 1)) {
          levelEnd = 1;
          levelFade = 16;
          levelFadeDir = 0;
        }
        if (nP(playerCode == 2)) {
          Tile *pTile = terrainAt(ply.getInfo().x, ply.getInfo().z);
          if (pTile->type == 2) inWater = 1;
          delete pTile;
          deaths++;
        }
        if (nP(playerCode == 3)) {
          SDL_WaitThread(thread, NULL);
          thread = nullptr;
          for (int i = 0; i < particleCount; i++) {
            if (particles[i]) {
              delete particles[i];
              particles[i] = nullptr;
            }
          }
          for (int i = 0; i < enemyCount; i++) {
            if (enemies[i]) {
              delete enemies[i];
              enemies[i] = nullptr;
            }
          }
          for (int i = 0; i < badFishCount; i++) {
            if (badFish[i]) {
              delete badFish[i];
              badFish[i] = nullptr;
            }
          }
          for (int i = 0; i < sharkCount; i++) {
            if (sharks[i]) {
              delete sharks[i];
              sharks[i] = nullptr;
            }
          }
          if (levelEnd) {
            screenNum++;
            levelFadeDir = 1;
          }
          PlayerInfo info = ply.getInfo();
          StartPos start = loadLevel(levelName, screenNum);
          if (start.levelExists) {
            info.x = start.x;
            info.z = start.y;
            info.xVel = 0;
            info.zVel = 0;
            info.onGround = 1;
            info.faceAnim = 0;
            setSwitch(0, 1);
            inWater = 0;
            ply.setInfo(info);
            if (levelEnd) ply.updateCamera(cam, ctrls.ctrls[0], 1);
          } else state = SWIN;
          levelEnd = 0;
        }
      } else {
        if (ctrls.ctrls[0]->b & CTRL_ACTION_MENU_BACK) state = SLVLSELECT;
        if (!assistModeFlip && ctrls.ctrls[0]->b & CTRL_ACTION_MENU_SPECIAL) assistMode = !assistMode;
        assistModeFlip = ctrls.ctrls[0]->b & CTRL_ACTION_MENU_SPECIAL;
      }
      ply.updateCamera(cam, ctrls.ctrls[0], 0);
      if (pause && ctrls.ctrls[0]->screenshot != ctrls.ctrls[0]->lastScreenshot) {
        if (ctrls.ctrls[0]->screenshot) {
          win.screenshot();
          lastEffects = simpleEffects;
          lastGui = hideGui;
          simpleEffects = 1;
          hideGui = 1;
        } else {
          simpleEffects = lastEffects;
          hideGui = lastGui;
        }
      }
      }
      break;
    case SWIN:
      if (stateChange) {
        levelFade = 1;
        std::fstream data;
        std::stringstream path;
        path << "levels/" << levelName << "/goals";
        data.open(pathForData(path.str().c_str()), std::ios::in);
        if (data) {
          data >> goalFrames1;
          data >> goalFrames2;
        } else {
          goalFrames1 = 60;
          goalFrames2 = 60;
        }
        data.close();
        grade = assistUsed ? 0 : (gameFrames > goalFrames1 ? 1 : (gameFrames > goalFrames2 ? 2 : 3));
        /*path.str("");
        path << "highscore-" << levelName;
        data.open(pathForSaves(path.str().c_str()), std::ios::in);
        if (data) data >> highScore;
        else highScore = -1;
        data.close();
        if (gameFrames < highScore && !assistUsed) {
          data.open(pathForSaves(path.str().c_str()), std::ios::out);
          if (data) data << gameFrames;
          data.close();
        }*/
        scores = getScoresForLevel(levelName);
        if (scores.valid == '~') {
          LevelScores lastScores = scores;
          if ((gameFrames < scores.highScore || scores.assistUsed) && !assistUsed) scores.highScore = gameFrames;
          if (grade > scores.grade) scores.grade = grade;
          scores.plays++;
          if (!assistUsed) scores.assistUsed = 0;
          setScoresForLevel(levelName, scores);
        } else {
          scores.valid = '~';
          scores.highScore = gameFrames;
          scores.grade = grade;
          scores.plays = 1;
          scores.assistUsed = assistUsed;
          setScoresForLevel(levelName, scores);
        }
        //resetAmbience();
      }
      if (ctrls.ctrls[0]->b & CTRL_ACTION_MENU_OK && levelFade) levelFade = 0;
      if (!(ctrls.ctrls[0]->b & CTRL_ACTION_MENU_OK) && !levelFade) {
        state = SLVLSELECT;
        if (selectedLevel == lockedLevel - 1) lockedLevel++;
      }
      break;
    case SOPTIONS:
      if (stateChange) {
        levelFade = 0;
        optionAction = 0;
        option = 0;
      }
      if (ctrls.ctrls[0]->b & CTRL_ACTION_MENU_BACK) state = SLVLSELECT;
      if (ctrls.ctrls[0]->aY < -63) {
        if (!levelFade) option--;
        levelFade = 1;
      } else if (ctrls.ctrls[0]->aY > 63) {
        if (!levelFade) option++;
        levelFade = 1;
      } else if (ctrls.ctrls[0]->b & CTRL_ACTION_MENU_OK) {
        if (!levelFade) optionAction = 1;
        levelFade = 1;
      } else levelFade = 0;
      if (optionAction && !levelFade) {
        switch (option) {
          case 0:
            state = SLVLSELECT;
            break;
          case 1:
            rumble = !rumble;
            ctrlSetRumble(rumble);
            ctrlRumble(*ctrls.ctrls[0], 1.0f, 1000);
            break;
          case 2:
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
            break;
          case 3:
            lowRender = !lowRender;
            win.setLowRender(lowRender);
            //win.resizeScreen(win.info.rW, win.info.rH, fullscreen);
            break;
          case 4:
            simpleEffects = !simpleEffects;
            break;
          case 5:
            enableParticles = !enableParticles;
            break;
          case 6:
            running = 0;
            break;
        }
        optionAction = 0;
      }
      if (option == -1) option = 6;
      if (option == 7) option = 0;
      break;
  }
  #if killWithHome == 1
    if (ctrls.ctrls[0]->home) running = 0;
  #endif
}

void render() {
  SDL_Rect rect;
  int w = 0, h = 0, lastW = 0, lastH = 0;
  switch (state) {
    case SLVLSELECT:
      rect = {0, 0, intResX * win.info.wRatio, intResY * win.info.hRatio};
      if (stagePreview) SDL_RenderCopy(win.getRender(), stagePreview, NULL, &rect);
      else SDL_RenderCopy(win.getRender(), unfinishedPreview, NULL, &rect);
      dispName = renderTextBg(win, levelName, 0, 0);
      SDL_QueryTexture(dispName, NULL, NULL, &w, &h);
      rect = {16 * win.info.wRatio, ((intResY - 16) - h) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
      SDL_RenderCopy(win.getRender(), dispName, NULL, &rect);
      SDL_DestroyTexture(dispName);
      if (selectedLevel + 1 < lockedLevel) {
        if (scores.assistUsed) dispName = renderTextBg(win, "High Score: ??:??.??", 1, 0);
        else {
          char timerText[64];
          memset(timerText, 0, sizeof(timerText));
          float secs = float(scores.highScore) / 60;
          int mins = secs / 60;
          while (secs >= 60) secs -= 60;
          int sec10 = secs / 10;
          while (secs >= 10) secs -= 10;
          sprintf(timerText, "High Score: %.2i:%i%.2f", mins, sec10, secs);
          dispName = renderTextBg(win, timerText, 1, 0);
        }
        SDL_QueryTexture(dispName, NULL, NULL, &w, &h);
        rect = {16 * win.info.wRatio, ((intResY - 40) - h) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
        SDL_RenderCopy(win.getRender(), dispName, NULL, &rect);
        SDL_DestroyTexture(dispName);
        for (int i = 0; i < 3; i++) {
          SDL_Rect clip = {((getTimer() / 8) % 4) * 16, (scores.grade >= i + 1 ? 1 : getTimer() % 2) * 16, 16, 16};
          rect = {(16 + i * 16) * win.info.wRatio, (intResY - 56 - h) * win.info.hRatio, 16 * win.info.wRatio, 16 * win.info.hRatio};
          SDL_RenderCopy(win.getRender(), starImg, &clip, &rect);
        }
      }
      if (!hideGui) {
        SDL_DestroyTexture(ctrlDisplay);
        char ctrlText[128] = "Move:@MENUMOVE@ Start:@MENUOK@";
        ctrlDisplay = renderTextBg(win, ctrlText, 0, 0);
        SDL_QueryTexture(ctrlDisplay, NULL, NULL, &w, &h);
        rect = {(intResX - 16 - w) * win.info.wRatio, ((intResY - 16) - h) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
        SDL_RenderCopy(win.getRender(), ctrlDisplay, NULL, &rect);
        SDL_DestroyTexture(ctrlDisplay);
        memset(ctrlText, 0, sizeof(ctrlText));
        sprintf(ctrlText, "Turn Assist Mode %s:@MENUSPECIAL@", assistMode ? "off" : "on");
        ctrlDisplay = renderTextBg(win, ctrlText, 1, assistMode, 3);
        SDL_QueryTexture(ctrlDisplay, NULL, NULL, &w, &h);
        rect = {(intResX - 16 - w) * win.info.wRatio, 16 * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
        SDL_RenderCopy(win.getRender(), ctrlDisplay, NULL, &rect);
        SDL_DestroyTexture(ctrlDisplay);
        memset(ctrlText, 0, sizeof(ctrlText));
        sprintf(ctrlText, "Options:@MENUSUBMENU@");
        ctrlDisplay = renderTextBg(win, ctrlText, 1, 0, 3);
        SDL_QueryTexture(ctrlDisplay, NULL, NULL, &w, &h);
        rect = {16 * win.info.wRatio, 16 * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
        SDL_RenderCopy(win.getRender(), ctrlDisplay, NULL, &rect);
      }
      break;
    case SINTRO:
      if (introState == 1) {
        dispName = renderTextBg(win, introText, 0, 0);
        SDL_QueryTexture(dispName, NULL, NULL, &w, &h);
        rect = {((intResX / 2) - (w / 2)) * win.info.wRatio, ((intResY / 2) - (h / 2)) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
        SDL_RenderCopy(win.getRender(), dispName, NULL, &rect);
        SDL_DestroyTexture(dispName);
      }
      if (introState > 2) {
        dispName = renderTextBg(win, introText, 0, (introState == 4) ? ((introFrames / 8) % 2) : 0, 3);
        SDL_QueryTexture(dispName, NULL, NULL, &w, &h);
        rect = {((intResX / 2) - (w / 2)) * win.info.wRatio, ((intResY / 2) - (h / 2)) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
        SDL_RenderCopy(win.getRender(), dispName, NULL, &rect);
        SDL_DestroyTexture(dispName);
      }
      break;
    case SGAME:
      /*if ((ctrls.ctrls[0]->a2X || ctrls.ctrls[0]->a2Y) && !lowDetail && 0) {
        for (int i = 0; i < 160; i++) {
          for (int j = 0; j < 90; j++) {
            SDL_Rect dotRect = {i * 4 * win.info.wRatio, j * 4 * win.info.hRatio, 4 * win.info.wRatio, 4 * win.info.hRatio};
            setColorFromPalette(win, (i * i + j + getTimer() / 4) % 4);
            SDL_RenderFillRect(win.getRender(), &dotRect);
          }
        }
      }*/
      renderTerrain(win, cam, ply, simpleEffects, -1);
      renderTerrain(win, cam, ply, simpleEffects, 0);
      for (int i = 0; i < enemyCount; i++) {
        if (enemies[i]) enemies[i]->render(win, cam);
      }
      for (int i = 0; i < badFishCount; i++) {
        if (badFish[i]) badFish[i]->render(win, cam, simpleEffects);
      }
      for (int i = 0; i < sharkCount; i++) {
        if (sharks[i]) sharks[i]->render(win, cam, simpleEffects);
      }
      ply.render(win, cam, ctrls.ctrls[0], simpleEffects, 0);
      renderTerrain(win, cam, ply, simpleEffects, 1);
      if (enableParticles) {
        for (int i = 0; i < particleCount; i++) {
          if (particles[i]) particles[i]->render(win, cam, simpleEffects);
        }
      }
      ply.render(win, cam, ctrls.ctrls[0], simpleEffects, 1);
      if (yP(levelFade)) {
        if (!simpleEffects)  {
          for (int i = 0; i < intResX; i++) {
            for (int j = 0; j < intResY; j++) {
              if ((i + j + getTimer()) % (levelFade ? levelFade : 1) == 0) {
                setColorFromPalette(win, ((i / 4) * (i / 4) + (j / 4) - (getTimer() / 8)) % 4);
                rect = {i * win.info.wRatio, j * win.info.hRatio, win.info.wRatio, win.info.hRatio};
                SDL_RenderFillRect(win.getRender(), &rect);
              }
            }
          }
        }
      }
      if (nP(!hideGui)) {
        float secs = float(gameFrames) / 60;
        int mins = secs / 60;
        while (secs >= 60) secs -= 60;
        int sec10 = secs / 10;
        while (secs >= 10) secs -= 10;
        char timerText[32];
        memset(timerText, 0, sizeof(timerText));
        sprintf(timerText, "Time: %.2i:%i%.2f", mins, sec10, secs);
        SDL_Texture *timerImg = renderTextBg(win, timerText, 1, 2, 1);
        SDL_QueryTexture(timerImg, NULL, NULL, &w, &h);
        rect = {((intResX - 16) - w) * win.info.wRatio, 16 * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
        SDL_RenderCopy(win.getRender(), timerImg, NULL, &rect);
        SDL_DestroyTexture(timerImg);
        SDL_DestroyTexture(ctrlDisplay);
        char text[128];
        sprintf(text, "Move:@GAMEMOVE@ Jump:@GAMEJUMP@ Run:@GAMERUN@ %s ", pause ? "Exit to Menu:@MENUBACK@  " : "");
        ctrlDisplay = renderTextBg(win, text, 1, 2, 1);
        SDL_QueryTexture(ctrlDisplay, NULL, NULL, &w, &h);
        rect = {16 * win.info.wRatio, ((intResY - 16) - h) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
        SDL_RenderCopy(win.getRender(), ctrlDisplay, NULL, &rect);
        SDL_DestroyTexture(ctrlDisplay);
        if (pause) {
          memset(text, 0, sizeof(text));
          sprintf(text, "%s Look:@GAMELOOK@ Screenshot:@SCREENSHOT@  ", assistMode ? "Triple Jump:@GAMEJUMP@x 3" : "Double Jump:@GAMEJUMP@x 2");
          ctrlDisplay = renderTextBg(win, text, 1, 2, 1);
          SDL_QueryTexture(ctrlDisplay, NULL, NULL, &w, &h);
          rect = {16 * win.info.wRatio, ((intResY - 48) - h) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
          SDL_RenderCopy(win.getRender(), ctrlDisplay, NULL, &rect);
          SDL_DestroyTexture(ctrlDisplay);
          memset(text, 0, sizeof(text));
          sprintf(text, "Turn Assist Mode %s:@MENUSPECIAL@", assistMode ? "off" : "on");
          ctrlDisplay = renderTextBg(win, text, 1, 2 + assistMode, 1);
          SDL_QueryTexture(ctrlDisplay, NULL, NULL, &w, &h);
          rect = {16 * win.info.wRatio, 16 * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
          SDL_RenderCopy(win.getRender(), ctrlDisplay, NULL, &rect);
        }
      }
      break;
    case SWIN:
      {
        if (stateChange) levelFade = 0;
        SDL_Rect rect;
        for (int i = 0; i < intResX / 4; i++) {
          for (int j = 0; j < intResY / 4; j++) {
            setColorFromPalette(win, ((i * i + j - (getTimer() / 8)) % 4) / 2 + 1);
            rect = {i * 4 * win.info.wRatio, j * 4 * win.info.hRatio, 4 * win.info.wRatio, 4 * win.info.hRatio};
            SDL_RenderFillRect(win.getRender(), &rect);
          }
        }
        lastW = 0, lastH = 0;
        float secs = float(gameFrames) / 60;
        int mins = secs / 60;
        while (secs >= 60) secs -= 60;
        int sec10 = secs / 10;
        while (secs >= 10) secs -= 10;
        char timerText[32];
        memset(timerText, 0, sizeof(timerText));
        sprintf(timerText, "Time: %.2i:%i%.2f", mins, sec10, secs);
        SDL_Texture *timerImg = renderTextBg(win, timerText, 1, 0, 3);
        SDL_QueryTexture(timerImg, NULL, NULL, &w, &h);
        lastH = h * -4;
        rect = {((intResX / 2) - (w / 2)) * win.info.wRatio, ((intResY / 2) - (h / 2) + lastH) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
        SDL_RenderCopy(win.getRender(), timerImg, NULL, &rect);
        SDL_DestroyTexture(timerImg);
        lastH += h;
        memset(timerText, 0, sizeof(timerText));
        if (assistUsed) strcpy(timerText, "No high score in assist mode!");
        else if (gameFrames <= scores.highScore) strcpy(timerText, "New High Score!");
        else {
          secs = float(scores.highScore) / 60;
          mins = secs / 60;
          while (secs >= 60) secs -= 60;
          sec10 = secs / 10;
          while (secs >= 10) secs -= 10;
          sprintf(timerText, "High Score: %.2i:%i%.2f", mins, sec10, secs);
        }
        timerImg = renderTextBg(win, timerText, 1, 0, 3);
        SDL_QueryTexture(timerImg, NULL, NULL, &w, &h);
        rect = {((intResX / 2) - (w / 2)) * win.info.wRatio, ((intResY / 2) - (h / 2) + lastH) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
        SDL_RenderCopy(win.getRender(), timerImg, NULL, &rect);
        SDL_DestroyTexture(timerImg);
        lastH += h;
        memset(timerText, 0, sizeof(timerText));
        sprintf(timerText, "Deaths: %i", deaths);
        timerImg = renderTextBg(win, timerText, 1, 0, 3);
        SDL_QueryTexture(timerImg, NULL, NULL, &w, &h);
        rect = {((intResX / 2) - (w / 2)) * win.info.wRatio, ((intResY / 2) - (h / 2) + lastH + 2) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
        SDL_RenderCopy(win.getRender(), timerImg, NULL, &rect);
        SDL_DestroyTexture(timerImg);
        lastH += h;
        if (!starImg) {
          memset(timerText, 0, sizeof(timerText));
          sprintf(timerText, "Score: %i", grade);
          timerImg = renderTextBg(win, timerText, 1, 0, 3);
          SDL_QueryTexture(timerImg, NULL, NULL, &w, &h);
          rect = {((intResX / 2) - (w / 2)) * win.info.wRatio, ((intResY / 2) - (h / 2) + lastH + 2) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
          SDL_RenderCopy(win.getRender(), timerImg, NULL, &rect);
          SDL_DestroyTexture(timerImg);
          lastH += h;
        } else {
          lastH += 8;
          for (int i = 0; i < 3; i++) {
            if (grade < i + 1 && getTimer() % 2) continue;
            SDL_Rect clip = {((getTimer() / 8) % 4) * 16, (scores.grade >= i + 1) * 16, 16, 16};
            rect = {((intResX / 2) - 47 + i * 32) * win.info.wRatio, ((intResY / 2) - 7 + lastH + 2) * win.info.hRatio, 32 * win.info.wRatio, 32 * win.info.hRatio};
            SDL_RenderCopy(win.getRender(), starImg, &clip, &rect);
          }
          lastH += 40;
        }
        memset(timerText, 0, sizeof(timerText));
        secs = float(grade == 1 ? goalFrames1 : goalFrames2) / 60;
        mins = secs / 60;
        while (secs >= 60) secs -= 60;
        sec10 = secs / 10;
        while (secs >= 10) secs -= 10;
        if (grade == 0) sprintf(timerText, "Next Star: No Assist Mode!");
        else if (grade == 3) sprintf(timerText, "Go to next stage!");
        else sprintf(timerText, "Next Star: Beat in %.2i:%i%.2f", mins, sec10, secs);
        timerImg = renderTextBg(win, timerText, 1, 0, 3);
        SDL_QueryTexture(timerImg, NULL, NULL, &w, &h);
        rect = {((intResX / 2) - (w / 2)) * win.info.wRatio, ((intResY / 2) - (h / 2) + lastH + 2) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
        SDL_RenderCopy(win.getRender(), timerImg, NULL, &rect);
        SDL_DestroyTexture(timerImg);
        lastH += h;
        timerImg = renderTextBg(win, "Press @MENUOK@", 1, 0, 3);
        SDL_QueryTexture(timerImg, NULL, NULL, &w, &h);
        rect = {((intResX / 2) - (w / 2)) * win.info.wRatio, ((intResY / 4) * 3 - (h / 2)) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
        SDL_RenderCopy(win.getRender(), timerImg, NULL, &rect);
        SDL_DestroyTexture(timerImg);
      }
      break;
    case SOPTIONS:
      SDL_Rect rect;
      char timerText[32];
      SDL_Texture *timerImg = renderTextBg(win, "Back to level select (or press @MENUBACK@)", 1, option == 0, 3);
      SDL_QueryTexture(timerImg, NULL, NULL, &w, &h);
      lastH = 0;
      rect = {48 * win.info.wRatio, (48 + lastH) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
      SDL_RenderCopy(win.getRender(), timerImg, NULL, &rect);
      SDL_DestroyTexture(timerImg);
      lastH += h;
      memset(timerText, 0, sizeof(timerText));
      sprintf(timerText, "Rumble: %s", rumble ? "on" : "off");
      timerImg = renderTextBg(win, timerText, 1, option == 1, 3);
      SDL_QueryTexture(timerImg, NULL, NULL, &w, &h);
      rect = {48 * win.info.wRatio, (48 + lastH) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
      SDL_RenderCopy(win.getRender(), timerImg, NULL, &rect);
      SDL_DestroyTexture(timerImg);
      lastH += h;
      timerImg = renderTextBg(win, "Toggle fullscreen", 1, option == 2, 3);
      SDL_QueryTexture(timerImg, NULL, NULL, &w, &h);
      rect = {48 * win.info.wRatio, (48 + lastH) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
      SDL_RenderCopy(win.getRender(), timerImg, NULL, &rect);
      SDL_DestroyTexture(timerImg);
      lastH += h;
      memset(timerText, 0, sizeof(timerText));
      sprintf(timerText, "Resolution: %s", win.info.lowRender ? "640 * 360" : "Native");
      timerImg = renderTextBg(win, timerText, 1, option == 3, 3);
      SDL_QueryTexture(timerImg, NULL, NULL, &w, &h);
      rect = {48 * win.info.wRatio, (48 + lastH) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
      SDL_RenderCopy(win.getRender(), timerImg, NULL, &rect);
      SDL_DestroyTexture(timerImg);
      lastH += h;
      memset(timerText, 0, sizeof(timerText));
      sprintf(timerText, "Effects: %s", simpleEffects ? "Simple" : "Cool");
      timerImg = renderTextBg(win, timerText, 1, option == 4, 3);
      SDL_QueryTexture(timerImg, NULL, NULL, &w, &h);
      rect = {48 * win.info.wRatio, (48 + lastH) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
      SDL_RenderCopy(win.getRender(), timerImg, NULL, &rect);
      SDL_DestroyTexture(timerImg);
      lastH += h;
      memset(timerText, 0, sizeof(timerText));
      sprintf(timerText, "Particles: %s", enableParticles ? "All" : "None");
      timerImg = renderTextBg(win, timerText, 1, option == 5, 3);
      SDL_QueryTexture(timerImg, NULL, NULL, &w, &h);
      rect = {48 * win.info.wRatio, (48 + lastH) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
      SDL_RenderCopy(win.getRender(), timerImg, NULL, &rect);
      SDL_DestroyTexture(timerImg);
      lastH += h;
      timerImg = renderTextBg(win, "Exit game", 1, option == 6, 3);
      SDL_QueryTexture(timerImg, NULL, NULL, &w, &h);
      rect = {48 * win.info.wRatio, (48 + lastH) * win.info.hRatio, w * win.info.wRatio, h * win.info.hRatio};
      SDL_RenderCopy(win.getRender(), timerImg, NULL, &rect);
      SDL_DestroyTexture(timerImg);
      lastH += h;
      rect = {16 * win.info.wRatio, (48 + h * option) * win.info.hRatio, 16 * win.info.wRatio, 16 * win.info.hRatio};
      SDL_Rect clip = {((getTimer() / 8) % 4) * 16, 0, 16, 16};
      SDL_RenderCopy(win.getRender(), starImg, &clip, &rect);
      break;
  }
  win.update();
}

void end() {
  if (superThread){
    loopRunning = 0;
    SDL_WaitThread(mainThread, NULL);
  }
  for (int i = 0; i < particleCount; i++) {
    if (particles[i]) delete particles[i];
  }
  for (int i = 0; i < enemyCount; i++) {
    if (enemies[i]) delete enemies[i];
  }
  SDL_DestroyTexture(dispName);
  SDL_DestroyTexture(ctrlDisplay);
  unloadFonts();
  unloadTerrain();
  enemyFreeRes();
  SDL_DestroyTexture(stagePreview);
  SDL_DestroyTexture(unfinishedPreview);
  ply.end();
  win.end();
  ctrlClose(ctrls);
  Mix_FreeChunk(menuMus);
  Mix_FreeChunk(gameMus);
  //freeAmbience();
  Mix_CloseAudio();
  SDL_Quit();
  fstream data;
  data.open(pathForSaves("settings.cfg"), ios::out);
  if (data) saveOptions(data);
  data.close();
  data.open(pathForSaves("complete"), ios::out | ios::binary);
  if (data) data.write((char *)&lockedLevel, sizeof(lockedLevel));
  data.close();
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
  fstream settings;
  settings.open(pathForSaves("settings.cfg"), ios::in);
  if (settings) {
    settings.seekg(0, settings.end);
    int length = settings.tellg() + 1L;
    settings.seekg(0, settings.beg);
    if (length % 2 == 1) {
      for (int i = 0; i < length / 2; i++) {
        char option = 0;
        char active = 0;
        settings.read(&option, 1);
        settings.read(&active, 1);
        //SDL_Log("Option %c is '%c'", option, active);
        doOption(option, active == 'y');
      }
    } else SDL_Log("Warning! Options file is in incorrect format.");
  }
  settings.close();
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
        if (!superThread && duration_cast<microseconds>(high_resolution_clock::now() - lastLoop).count() > 15000) {
          for (int i = 0; i < loopCount; i++) loop();
          stateChange = state != lastState;
          lastState = state;
          lastLoop = high_resolution_clock::now();
        }
        rendering = 1;
        render();
        rendering = 0;
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
