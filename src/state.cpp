#include "state.h"
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
using namespace std;
using json = nlohmann::json;

extern CtrlList ctrls;
extern Window win;
extern Player ply;
extern Particle *particles[particleCount];
extern Enemy *enemies[enemyCount];
extern BadFish *badFish[badFishCount];
extern Shark *sharks[sharkCount];
Camera cam = {0, 0};
extern char levelName[64];
int screenNum = 0;
bool levelEnd;
extern Mix_Chunk *menuMus, *gameMus;
extern bool running;
bool hasMoved = 0;
int selectedLevel = 0;
extern int lListLen;
extern json levelList;
extern bool pause;
extern bool lastPause;
extern bool lowDetail;
extern SDL_Thread *thread;
extern bool fullscreen, vsync, useKeyboard, enableParticles, rumble, heavyRumble, simpleEffects, hideGui, music, lowRender, skipIntro, fps30;
extern unsigned long lastFullToggle;
extern char name[];
extern char me[];
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
extern LevelScores scores;
extern int lockedLevel;
extern int sX, sY;

bool LevelSelectState::init() volatile {
  char previewPath[64];
  sprintf(previewPath, pathForData("screenshots/%s.png"), levelName);
  stagePreview = loadTexture(previewPath, win);
  if (!stagePreview) return 0;
  unfinishedPreview = loadTexture(pathForData("imgs/unfinished.png"), win);
  if (!unfinishedPreview) return 0;
  starImg = loadTexture(pathForData("imgs/star.png"), win);
  if (!starImg) return 0;
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
  return 1;
}

GameState *LevelSelectState::update() volatile {
  if (ctrls.ctrls[0]->b & CTRL_ACTION_MENU_SUBMENU) return new OptionsState;
  if (ctrls.ctrls[0]->b & CTRL_ACTION_MENU_OK && selectedLevel < lockedLevel) {
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
      return new PlayState;
    } else return nullptr;
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
  return nullptr;
}

void LevelSelectState::render() volatile {
  SDL_Rect rect = {0, 0, intResX * win.info.wRatio, intResY * win.info.hRatio};
  int w, h;
  if (stagePreview) SDL_RenderCopy(win.getRender(), stagePreview, NULL, &rect);
  else SDL_RenderCopy(win.getRender(), unfinishedPreview, NULL, &rect);
  SDL_Texture *dispName = renderTextBg(win, levelName, 0, 0);
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
}

void LevelSelectState::end() volatile {
  SDL_DestroyTexture(stagePreview);
  SDL_DestroyTexture(unfinishedPreview);
}



bool PlayState::init() volatile {
  Mix_HaltChannel(-1);
  if (music) Mix_PlayChannel(-1, gameMus, -1);
  deaths = 0;
  levelEnd = 0;
  return 1;
}

GameState *PlayState::update() volatile {
  threadFunc();
  if (nP(levelFade > 2 && !levelFadeDir && getTimer() % 4 == 0)) levelFade--;
  if (nP(levelFadeDir && getTimer() % 4 == 0)) {
    levelFade++;
    if (levelFade > 16) {
      levelFade = 0;
      levelFadeDir = 0;
    }
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
      } else return new WinState;
      levelEnd = 0;
    }
  } else {
    if (ctrls.ctrls[0]->b & CTRL_ACTION_MENU_BACK) return new LevelSelectState;
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
  return nullptr;
}

void PlayState::threadFunc() volatile {
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
}

void PlayState::render() volatile {
  SDL_Rect rect;
  int w, h, lastW, lastH;
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
    char text[128];
    sprintf(text, "Move:@GAMEMOVE@ Jump:@GAMEJUMP@ Run:@GAMERUN@ %s ", pause ? "Exit to Menu:@MENUBACK@  " : "");
    SDL_Texture *ctrlDisplay = renderTextBg(win, text, 1, 2, 1);
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
      SDL_DestroyTexture(ctrlDisplay);
    }
  }
}

void PlayState::end() volatile {

}



bool IntroState::init() volatile {
  return 1;
}

GameState *IntroState::update() volatile {
  if (skipIntro || ctrls.ctrls[0]->b & CTRL_ACTION_MENU_BACK) return new LevelSelectState;
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
    }
  }
  if (introState == 4 && introFrames > 120) {
    introState = 5;
    introFrames = 0;
  }
  if (introState == 5 && introFrames > 60) return new LevelSelectState;
  introFrames++;
  return nullptr;
}

void IntroState::render() volatile {
  SDL_Rect rect;
  int w, h, lastW, lastH;
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
}

void IntroState::end() volatile {

}



bool WinState::init() volatile {
  levelFade = 1;
  FILE *file;
  char path[32];
  memset(path, 0, sizeof(path));
  sprintf(path, "levels/%s/goals", levelName);
  file = fopen(pathForData(path), "r");
  if (file) {
    char data = '0', buffer[8];
    int i = 0;
    memset(buffer, 0, sizeof(buffer));
    while (data != '\n') {
      fread(&data, sizeof(data), 1, file);
      buffer[i] = data;
      i++;
    }
    goalFrames1 = atoi(buffer);
    memset(buffer, 0, sizeof(buffer));
    data = '0';
    i = 0;
    while (data != '\n') {
      if (feof(file)) {
        goalFrames2 = 60;
        break;
      }
      fread(&data, sizeof(data), 1, file);
      buffer[i] = data;
      i++;
    }
    goalFrames2 = atoi(buffer);
    fclose(file);
  } else {
    goalFrames1 = 60;
    goalFrames2 = 60;
  }
  grade = assistUsed ? 0 : (gameFrames > goalFrames1 ? 1 : (gameFrames > goalFrames2 ? 2 : 3));
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
  levelFade = 1;
  return 1;
}

GameState *WinState::update() volatile {
  if (ctrls.ctrls[0]->b & CTRL_ACTION_MENU_OK && levelFade) levelFade = 0;
  if (!(ctrls.ctrls[0]->b & CTRL_ACTION_MENU_OK) && !levelFade) {
    if (selectedLevel == lockedLevel - 1) lockedLevel++;
    return new LevelSelectState;
  }
  return nullptr;
}

void WinState::render() volatile {
  SDL_Rect rect;
  int w, h, lastW, lastH;
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

void WinState::end() volatile {

}



bool OptionsState::init() volatile {
  levelFade = 0;
  optionAction = 0;
  option = 0;
  return 1;
}

GameState *OptionsState::update() volatile {
  if (ctrls.ctrls[0]->b & CTRL_ACTION_MENU_BACK) return new LevelSelectState;
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
        return new LevelSelectState;
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
  return nullptr;
}

void OptionsState::render() volatile {
  SDL_Rect rect;
  int w, h, lastW, lastH;
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
}

void OptionsState::end() volatile {

}