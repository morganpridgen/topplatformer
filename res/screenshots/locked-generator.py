import pygame, os, sys, glob

os.system("rm level*-locked.png")
pygame.init()
files = glob.glob("level*.png")
palette = [(0, 0, 0), (8, 0xa0, 0), (0x20, 0xf0, 0), (0xa0, 0xff, 0x40)]

for file in files:
  img = pygame.image.load(file)
  for i in range(img.get_width()):
    for j in range(img.get_height()):
      for k in range(len(palette)):
        if k == 0: continue
        if img.get_at((i, j)) == palette[k]: img.set_at((i, j), palette[k - 1])
  newFile = list(file)
  for i in range(4): newFile.pop(len(newFile) - 1)
  pygame.image.save(img, "".join(["".join(newFile), "-locked.png"]))