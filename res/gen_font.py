import pygame
from pygame.locals import *

pygame.init()
for fontSize in [14, 22]:
  font = pygame.font.Font("terminusbold.ttf", fontSize)
  renderString = ""
  for i in range(32, 127): renderString = "".join([renderString, chr(i)])
  testChr = font.render("?", 0, (255, 255, 255))
  charSize = testChr.get_size()
  print(charSize)
  outSurf = pygame.Surface((charSize[0] * len(renderString), max(charSize[1], 16) * 4), SRCALPHA)
  colors = [[0xff, 0xff, 0xff], [0x08, 0xa0, 0x00], [0x20, 0xf0, 0x00], [0xa0, 0xff, 0x40]]
  for i in range(4): outSurf.blit(font.render(renderString, 0, colors[i], pygame.Color(0, 0, 0, 0)), (0, max(charSize[1], 16) * i))
  for i in range(outSurf.get_size()[0]):
    for j in range(outSurf.get_size()[1]):
      if outSurf.get_at((i, j)) == (0, 0, 0): outSurf.set_at((i, j), (0, 0, 0, 0))
      if outSurf.get_at((i, j)) == (0xff, 0xff, 0xff): outSurf.set_at((i, j), (0, 0, 0))
  pygame.image.save(outSurf, "font%i.png" % fontSize)
pygame.quit()