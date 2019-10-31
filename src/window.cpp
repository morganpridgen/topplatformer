#include "window.h"

#include <cstdio>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "helpers.h"

Window::Window() {
  display = nullptr;
  renderer = nullptr;
  winTexture = nullptr;
}

Window::~Window() {
  end();
}

void Window::end() {
  killScreen();
}

void Window::killScreen() {
  if (renderer) SDL_DestroyRenderer(renderer);
  if (display) SDL_DestroyWindow(display);
  //SDL_DestroyTexture(winTexture);
}

void Window::resizeScreen(int resX, int resY, bool fullscreen) {
  if (!fullscreen) SDL_SetWindowFullscreen(display, 0);
  SDL_SetWindowSize(display, resX, resY);
  if (fullscreen) SDL_SetWindowFullscreen(display, SDL_WINDOW_FULLSCREEN);
  /*info.w = (resX / intResX) * intResX;
  info.h = (resY / intResY) * intResY;*/
  info.rW = resX;
  info.rH = resY;
  if (!info.lowRender) {
    info.wRatio = resX / intResX;
    info.hRatio = resY / intResY;
    if (info.wRatio < info.hRatio) info.hRatio = info.wRatio;
    if (info.hRatio < info.wRatio) info.wRatio = info.hRatio;
    info.w = intResX * info.wRatio;
    info.h = intResY * info.hRatio;
    info.screenRect = {(resX / 2) - (info.w / 2), (resY / 2) - (info.h / 2), info.w, info.h};
  }
  /*if (toTexture) {
    SDL_DestroyTexture(winTexture);
    winTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, info.w, info.h);
  }*/
  SDL_RenderSetViewport(renderer, &info.screenRect);
  SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
  SDL_RenderClear(renderer);
  //SDL_RaiseWindow(display);
}

bool Window::makeScreen(int resX, int resY, char name[], bool fullscreen, bool vsync, bool lowRender) {
  int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS;
  display = SDL_CreateWindow(name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, resX, resY, flags);
  if (!display) {
    SDL_Log("[ERROR] SDL couldn't create a window. %s\n", SDL_GetError());
    return 0;
  }
  SDL_Surface *icon = IMG_Load(pathForData("icon.png"));
  SDL_SetWindowIcon(display, icon);
  SDL_FreeSurface(icon);
  flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE;
  if (vsync) flags |= SDL_RENDERER_PRESENTVSYNC;
  renderer = SDL_CreateRenderer(display, -1, flags);
  if (!renderer) {
    const char *error = SDL_GetError();
    renderer = SDL_GetRenderer(display);
    if (!renderer) {
      SDL_Log("[ERROR] SDL couldn't get a renderer from the window. (%s), (%s)\n", error, SDL_GetError());
      return 0;
    }
  }
  toTexture = SDL_RenderTargetSupported(renderer);
  /*info.w = (resX / intResX) * intResX;
  info.h = (resY / intResY) * intResY;*/
  info.lowRender = lowRender;
  info.rW = resX;
  info.rH = resY;
  if (lowRender) {
    info.wRatio = 1;
    info.hRatio = 1;
    info.w = intResX;
    info.h = intResY;
    info.screenRect = {0, 0, intResX, intResY};
    SDL_RenderSetLogicalSize(renderer, intResX, intResY);
  } else {
    info.wRatio = resX / intResX;
    info.hRatio = resY / intResY;
    if (info.wRatio < info.hRatio) info.hRatio = info.wRatio;
    if (info.hRatio < info.wRatio) info.wRatio = info.hRatio;
    info.w = intResX * info.wRatio;
    info.h = intResY * info.hRatio;
    info.screenRect = {(resX / 2) - (info.w / 2), (resY / 2) - (info.h / 2), info.w, info.h};
  }
  SDL_RenderSetIntegerScale(renderer, SDL_TRUE);
  /*if (toTexture) {
    winTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, info.w, info.h);
    SDL_SetRenderTarget(renderer, winTexture);
  }*/
  info.lowDetail = 0;
  SDL_RenderSetViewport(renderer, &info.screenRect);
  SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
  SDL_RenderClear(renderer);
  renderTick = 0;
  SDL_RendererInfo rInfo;
  SDL_GetRendererInfo(renderer, &rInfo);
  char dName[64];
  sprintf(dName, "%s (%s)", name, rInfo.name);
  SDL_SetWindowTitle(display, dName);
  return 1;
}

void Window::setLowRender(bool lowRender) {
  info.lowRender = lowRender;
  if (lowRender) {
    info.wRatio = 1;
    info.hRatio = 1;
    info.w = intResX;
    info.h = intResY;
    info.screenRect = {0, 0, intResX, intResY};
    SDL_RenderSetLogicalSize(renderer, intResX, intResY);
  } else {
    info.wRatio = info.rW / intResX;
    info.hRatio = info.rH / intResY;
    if (info.wRatio < info.hRatio) info.hRatio = info.wRatio;
    if (info.hRatio < info.wRatio) info.wRatio = info.hRatio;
    info.w = intResX * info.wRatio;
    info.h = intResY * info.hRatio;
    info.screenRect = {(info.rW / 2) - (info.w / 2), (info.rH / 2) - (info.h / 2), info.w, info.h};
    SDL_RenderSetLogicalSize(renderer, info.rW, info.rH);
  }
  SDL_RenderSetIntegerScale(renderer, SDL_TRUE);
  /*if (toTexture) {
    SDL_SetRenderTarget(renderer, NULL);
    SDL_DestroyTexture(winTexture);
    winTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, info.w, info.h);
    SDL_SetRenderTarget(renderer, winTexture);
  }*/
  SDL_RenderSetViewport(renderer, &info.screenRect);
  SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
  SDL_RenderClear(renderer);
}

void Window::update() {
  /*if (toTexture) {
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, winTexture, NULL, NULL);
  }*/
  /*if (nP(isScreenshot)) {
    SDL_Surface *screenshotImg = SDL_CreateRGBSurface(0, info.w, info.h, 32, 0, 0, 0, 0);
    SDL_RenderReadPixels(renderer, NULL, screenshotImg->format->format, screenshotImg->pixels, screenshotImg->pitch);
    IMG_SavePNG(screenshotImg, pathForSaves("screenshot.png"));
    SDL_FreeSurface(screenshotImg);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, winTexture, NULL, NULL);
    SDL_DestroyTexture(winTexture);
    isScreenshot = 0;
  }*/
  SDL_RenderPresent(renderer);
  SDL_RenderSetViewport(renderer, &info.screenRect);
  SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
  SDL_RenderClear(renderer);
  /*if (toTexture) {
    SDL_SetRenderTarget(renderer, winTexture);
    SDL_RenderClear(renderer);
  }*/
}

void Window::screenshot() {
  if (!toTexture) return;
  isScreenshot = 1;
  winTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, info.w, info.h);
  SDL_SetRenderTarget(renderer, winTexture);
}