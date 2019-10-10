#include "helpers.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "window.h"
#include "controller.h"
#ifdef __linux__
  #include <unistd.h>
#endif
#ifdef _WIN32
  #include <windows.h>
  #include <string.h>
  #define abs(x) (x<0?-x:x)
#endif

int rngVal = 0;
unsigned long timer = 0;
bool managingParticle = 0;

unsigned short rng() {
  unsigned short s0 = (unsigned char)rngVal << 8;
  s0 ^= rngVal;
  rngVal = ((s0 & 0xFF) << 8) | ((s0 & 0xFF00) >> 8);
  s0 = ((unsigned char)s0 >> 1) ^ rngVal;
  short s1 = (s0 >> 1) ^ 0xFF80;
  if ((s0 & 1) == 0) rngVal = s1 ^ 0x1FF4;
  else rngVal = s1 ^ 0x8180;
  return rngVal;
}

SDL_Texture* loadTexture(const char path[], Window &win) {
  SDL_Texture* newTexture = nullptr;
  SDL_Surface* loadedSurface = IMG_Load(path);
  if(nP(!loadedSurface)) SDL_Log("Unable to load image. %s\n", SDL_GetError());
  else {
    newTexture = SDL_CreateTextureFromSurface(win.getRender(), loadedSurface);
    if(!newTexture) SDL_Log("Unable to create texture. %s\n", SDL_GetError());
    SDL_FreeSurface(loadedSurface);
  }
  return newTexture;
}

void updateTimer() {
  timer++;
}

unsigned long getTimer() {
  return timer;
}

void setColorFromPalette(Window &win, int color) {
  if (color == 0) SDL_SetRenderDrawColor(win.getRender(), 0x00, 0x00, 0x00, 0xFF);
  if (color == 1) SDL_SetRenderDrawColor(win.getRender(), 0x08, 0xA0, 0x00, 0xFF);
  if (color == 2) SDL_SetRenderDrawColor(win.getRender(), 0x20, 0xF0, 0x00, 0xFF);
  if (color == 3) SDL_SetRenderDrawColor(win.getRender(), 0xA0, 0xFF, 0x40, 0xFF);
}

Hitbox::Hitbox(short x, short y, short w, short h) {
  box = {x - (w / 2) + 1, x + (w / 2), y - (h / 2) + 1, y + (h / 2)};
}

Hitbox::Hitbox() {
  box = {0, 0, 1, 1};
}

bool Hitbox::collide(Hitbox other) {
  return ((((box.xBot > other.box.xBot) && (box.xBot < other.box.xTop)) || ((box.xTop > other.box.xBot) && (box.xTop < other.box.xTop))) && (((box.yBot > other.box.yBot) && (box.yBot < other.box.yTop)) || ((box.yTop > other.box.yBot) && (box.yTop < other.box.yTop))));
}

void Hitbox::changeBox(short x, short y, short w, short h) {
  box = {x - (w / 2) + 1, x + (w / 2), y - (h / 2) + 1, y + (h / 2)};
}

Particle::Particle() {
  info = {0, 0, 0, 0, 0, 0, 3};
  timer = 960;
  active = 1;
  shiny = 0;
}

void Particle::update() {
  info.x += info.xVel;
  info.y += info.yVel;
  info.z += info.zVel;
  if (info.xVel > 0) info.xVel -= 4;
  if (info.xVel < 0) info.xVel += 4;
  if (info.zVel > 0) info.zVel -= 4;
  if (info.zVel < 0) info.zVel += 4;
  info.yVel += 4;
  if (info.y > 0) {
    info.y = -1;
    info.yVel *= -0.5;
  }
  timer--;
  if ((abs(info.xVel) < 4 && abs(info.zVel) < 4) || timer == 0) active = 0;
}

void Particle::render(Window &win, Camera cam, bool simpleEffects) {
  SDL_Rect rect = {(WORLDTOPIX(info.x, cam.x) - 0) * win.info.wRatio, (WORLDTOPIX(info.z + info.y, cam.y) - 0) * win.info.hRatio, 2 * win.info.wRatio,  2 * win.info.hRatio};
  if (rect.x < -2 * win.info.wRatio || rect.y < -2 * win.info.hRatio || rect.x > intResX * win.info.wRatio || rect.y > intResY * win.info.hRatio) return;
  setColorFromPalette(win, info.color);
  SDL_RenderFillRect(win.getRender(), &rect);
  if (yP(simpleEffects || !shiny)) return;
  for (int i = 0; i < 4 * win.info.wRatio; i++) {
    for (int j = 0; j < 4 * win.info.hRatio; j++) {
      if (int((WORLDTOPIX(info.x, cam.x) - 1) * win.info.wRatio + i + (WORLDTOPIX(info.z, cam.y) - 1) * win.info.hRatio + j + getTimer()) % 2 == 0) SDL_RenderDrawPoint(win.getRender(), (WORLDTOPIX(info.x, cam.x) - 1) * win.info.wRatio + i, (WORLDTOPIX(info.z, cam.y) - 1) * win.info.hRatio + j);
    }
  }
}

void addParticle(Particle *particles[particleCount], ParticleInfo newInfo, bool shiny) {
  while (managingParticle);
  managingParticle = 1;
  for (int i = 0; i < particleCount; i++) {
    if (!particles[i]) {
      particles[i] = new Particle;
      particles[i]->setInfo(newInfo);
      particles[i]->shiny = shiny;
      break;
    }
  }
  managingParticle = 0;
}

void addParticle(Particle *particles[particleCount], ParticleInfo newInfo) {
  addParticle(particles, newInfo, 0);
}

SDL_Surface *fontSmall = nullptr;
SDL_Surface *fontBig = nullptr;

bool loadFonts() {
  fontSmall = IMG_Load(pathForData("font14.png"));
  if (!fontSmall) return 0;
  fontBig = IMG_Load(pathForData("font22.png"));
  if (!fontBig) return 0;
  SDL_ConvertSurfaceFormat(fontSmall, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_ConvertSurfaceFormat(fontBig, SDL_PIXELFORMAT_RGBA32, 0);
  return 1;
}

void unloadFonts() {
  SDL_FreeSurface(fontSmall);
  SDL_FreeSurface(fontBig);
}

char specialToCtrl(char *specialChar) {
  if (!strcmp(specialChar, "MENUMOVE")) return ctrlActionToChr(CTRL_ACTIONNAME_MENU_MOVE);
  if (!strcmp(specialChar, "MENUOK")) return ctrlActionToChr(CTRL_ACTIONNAME_MENU_OK);
  if (!strcmp(specialChar, "MENUBACK")) return ctrlActionToChr(CTRL_ACTIONNAME_MENU_BACK);
  if (!strcmp(specialChar, "MENUSPECIAL")) return ctrlActionToChr(CTRL_ACTIONNAME_MENU_SPECIAL);
  if (!strcmp(specialChar, "MENUSUBMENU")) return ctrlActionToChr(CTRL_ACTIONNAME_MENU_SUBMENU);

  if (!strcmp(specialChar, "GAMEMOVE")) return ctrlActionToChr(CTRL_ACTIONNAME_GAME_MOVE);
  if (!strcmp(specialChar, "GAMELOOK")) return ctrlActionToChr(CTRL_ACTIONNAME_GAME_LOOK);
  if (!strcmp(specialChar, "GAMEJUMP")) return ctrlActionToChr(CTRL_ACTIONNAME_GAME_JUMP);
  if (!strcmp(specialChar, "GAMERUN")) return ctrlActionToChr(CTRL_ACTIONNAME_GAME_RUN);
  if (!strcmp(specialChar, "GAMESQUISH")) return ctrlActionToChr(CTRL_ACTIONNAME_GAME_SQUISH);
  if (!strcmp(specialChar, "GAMEPAUSE")) return ctrlActionToChr(CTRL_ACTIONNAME_GAME_PAUSE);

  if (!strcmp(specialChar, "SCREENSHOT")) return ctrlActionToChr(CTRL_ACTIONNAME_SCREENSHOT);
  return 0;
}

SDL_Surface *surfaceText(char *inText, bool isSmall, bool bg, int color) {
  char text[128];
  strcpy(text, inText);
  int i = 0;
  while (true) {
    if (nP(text[i] == 0)) break;
    if (text[i] == '@') {
      int j = i + 1;
      while (text[j] != '@') {
        j++;
      }
      char *specialChar = new char[j - i];
      for (int k = 0; k < j - i; k++) specialChar[k] = text[i + k + 1];
      specialChar[j - i - 1] = 0;
      text[i + 1] = specialToCtrl(specialChar);
      int k = i + 2;
      while (true) {
        text[k] = text[k + j - i - 1];
        if (text[k + j - i - 1] == 0) break;
        k++;
      }
    }
    i++;
  }
  SDL_Surface *font;
  if (isSmall) font = fontSmall;
  else font = fontBig;
  int chrW = font->w / (127 - 32);
  int chrH = font->h / 4;
  SDL_Surface *out = SDL_CreateRGBSurfaceWithFormat(0, chrW * (strlen(text) + 1), chrH, 32, SDL_PIXELFORMAT_RGBA32);
  i = 0;
  int charPos = 0;
  while (true) {
    if (nP(text[i] == 0)) break;
    if (text[i] == '@') {
      i++;
      if (!bg) {
        SDL_Surface *glyph = ctrlGetGlyphForButton(text[i], color);
        SDL_Rect rect = {charPos * chrW - (chrW - glyph->w) / 2, 0 + (chrH - glyph->h) / 2, glyph->w, glyph->h};
        SDL_BlitSurface(glyph, NULL, out, &rect);
        SDL_FreeSurface(glyph);
      }
      i++;
      charPos += 3;
      continue;
    }
    SDL_Rect clip = {(text[i] - 32) * chrW, color * chrH, chrW, chrH};
    SDL_Rect rect = {charPos * chrW, 0, chrW, chrH};
    SDL_BlitSurface(font, &clip, out, &rect);
    i++;
    charPos++;
  }
  return out;
}

SDL_Texture *renderText(Window &win, char *text, bool isSmall, int color) {
  SDL_Surface *out = surfaceText(text, isSmall, 0, color);
  SDL_Texture *textTexture = SDL_CreateTextureFromSurface(win.getRender(), out);
  SDL_FreeSurface(out);
  return textTexture;
}

SDL_Texture *renderTextBg(Window &win, char *text, bool isSmall, int color, int bgColor) {
  SDL_Surface *main = surfaceText(text, isSmall, 0, color);
  SDL_Surface *bg = surfaceText(text, isSmall, 1, bgColor);
  SDL_Surface *out = SDL_CreateRGBSurfaceWithFormat(0, main->w + 2, main->h + 2, 32, SDL_PIXELFORMAT_RGBA32);
  SDL_FillRect(out, NULL, SDL_MapRGBA(out->format, 0, 0, 0, 0));
  SDL_Rect rect = {0, 0, main->w, main->h};
  SDL_BlitSurface(bg, NULL, out, &rect);
  for (int i = 0; i < 2; i++) {
    rect.x++;
    SDL_BlitSurface(bg, NULL, out, &rect);
  }
  for (int i = 0; i < 2; i++) {
    rect.y++;
    SDL_BlitSurface(bg, NULL, out, &rect);
  }
  for (int i = 0; i < 2; i++) {
    rect.x--;
    SDL_BlitSurface(bg, NULL, out, &rect);
  }
  for (int i = 0; i < 2; i++) {
    rect.y--;
    SDL_BlitSurface(bg, NULL, out, &rect);
  }
  rect.x++;
  rect.y++;
  SDL_BlitSurface(main, NULL, out, &rect);
  SDL_Texture *textTexture = SDL_CreateTextureFromSurface(win.getRender(), out);
  SDL_FreeSurface(out);
  SDL_FreeSurface(main);
  SDL_FreeSurface(bg);
  return textTexture;
}

SDL_Texture *renderTextBg(Window &win, char *text, bool isSmall, int color) {
  return renderTextBg(win, text, isSmall, color, color == 0 ? 3 : 0);
}

#define devPath 0
char baseDir[128];
char savePath[128];
char dataPath[128];

void initPaths() {
  char tmpPath[128];
  #ifdef __linux__
    tmpPath[readlink("/proc/self/exe", tmpPath, 128)] = 0;
    realpath(tmpPath, baseDir);
  #else
    #ifdef _WIN32
      GetModuleFileName(NULL, tmpPath, 128);
      _fullpath(baseDir, tmpPath, 128);
    #endif
  #endif
  int i = 0;
  while (1) {
    if (baseDir[i] == 0) break;
    i++;
  }
  i--;
  baseDir[i] = 0;
  while (1) {
    i--;
    if (baseDir[i] == '/' || baseDir[i] == '\\') {
      baseDir[i] = 0;
      break;
    }
    baseDir[i] = 0;
  }
  sprintf(dataPath, "%s/res", baseDir);
  #if devPath == 1
    sprintf(savePath, "%s/saves", baseDir);
  #else
    memset(tmpPath, 0, 128);
    #ifdef __linux__
      char *xdgPath = getenv("XDG_DATA_HOME");
      if (xdgPath && strlen(xdgPath) > 0) {
        i = 0;
        while (1) {
          tmpPath[i] = xdgPath[i];
          i++;
          if (xdgPath[i] == ':' || xdgPath[i] == 0) {
            tmpPath[i] = 0;
            break;
          }
        }
      } else sprintf(tmpPath, "%s/.local/share", getenv("HOME"));
    #else
      #ifdef _WIN32
        strcpy(tmpPath, getenv("APPDATA"));
      #endif
    #endif
    sprintf(savePath, "%s/topplatformer", tmpPath);
  #endif
  SDL_Log("Saving data to %s", savePath);
  struct stat info;
  if (stat(savePath, &info) == 0 && info.st_mode & S_IFDIR) return;
  char cmd[128];
  sprintf(cmd, "mkdir \"%s\"", savePath);
  system(cmd);
}

char *pathForData(const char *file) {
  static char out[64];
  memset(out, 0, 64);
  sprintf(out, "%s/%s", dataPath, file);
  return out;
}

char *pathForSaves(const char *file) {
  static char out[64];
  memset(out, 0, 64);
  sprintf(out, "%s/%s", savePath, file);
  return out;
}