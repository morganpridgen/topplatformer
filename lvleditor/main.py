import sys, os, pygame, json
from pygame.locals import *

os.chdir("../res")
pygame.init()
w, h = 1280, 720
screen = pygame.display.set_mode((w, h))
clock = pygame.time.Clock()
keys = []
timer = 0
loop = 1
levelName = "uhoh"
try: levelName = sys.argv[1]
except: pass
screenNum = 0
try: screenNum = int(sys.argv[2])
except: pass
saveMode = "json"
try: saveMode = sys.argv[3]
except: pass
cam = [0, 0]
selectedTile = 1
maxTiles = 19

def lvlToLvlOut(layout, startPos):
  data = [ord("C"), ord("M")]
  palette = []
  for i in range(45):
    for j in range(80):
      if not int(layout[i][j]) in palette:
        if len(palette) < 8: palette.append(int(layout[i][j]))
        else: raise RuntimeError("Too many tile types!")
  while len(palette) < 8: palette.append(33)
  for i in range(8): data.append(palette[i])
  for i in range(2): data.append(int(startPos[i]))
  for i in range(45):
    for j in range(80):
      for k in range(8):
        if palette[k] == layout[i][j]:
          data.append(k)
          break;
  data.append(0)
  return data
def lvlToLvlIn(terrain):
  data = list(terrain)
  if data[0] != ord("C") or data[1] != ord("M"): raise RuntimeError("Invalid level format")
  palette = []
  for i in range(8): palette.append(data[i + 2])
  startPos = [data[10], data[11]]
  layout = []
  for i in range(45):
    layout.append([])
    for j in range(80):
      layout[i].append(palette[data[i * 80 + j + 12]])
  return (layout, startPos)
def lvlToJson(layout, startPos):
  data = []
  data.append("{\n  \"layout\": [\n")
  for i in range(45): data.append("    %s%s\n" % (str(layout[i]), "," if i != 44 else ""))
  data.append("  ],\n  \"startpos\": ")
  data.append(str(startPos))
  data.append("\n}")
  return "".join(data)
def ldImg(path, offset=0):
  outSurf = pygame.Surface((32, 32))
  outSurf.blit(pygame.image.load(path), (-offset, 0))
  return outSurf
def colorFromPalette(id):
  if id == 0: return (0x00, 0x00, 0x00)
  if id == 1: return (0x08, 0xa0, 0x00)
  if id == 2: return (0x20, 0xf0, 0x00)
  if id == 3: return (0xa0, 0xff, 0x40)
  return colorFromPalette(id % 4)
groundImg, wallImg, waterImg, goalImg, fakeBarrierImg, boosterImg, fireSpitterImg, fireImg, enemyImg, fishImg, sharkImg, switchImg, sBlock1Img, sBlock2Img, sTimeImg, sTKillImg, fanUpImg, fanOutImg, turboImg, firebarImg = ldImg("imgs/ground.png"), ldImg("imgs/barrier.png"), ldImg("imgs/water.png"), ldImg("imgs/goal.png"), ldImg("imgs/fakebarrier.png"), ldImg("imgs/fire.png"), ldImg("imgs/fire.png", 32), pygame.Surface((32, 32)), ldImg("imgs/enemy.png"), ldImg("imgs/badfish.png", 32), ldImg("imgs/shark.png"), ldImg("imgs/switch.png"), ldImg("imgs/switchblock.png"), ldImg("imgs/switchblock.png", 96), ldImg("imgs/switch.png", 128), ldImg("imgs/switch.png", 160), ldImg("imgs/fan.png"), ldImg("imgs/fan.png"), pygame.Surface((32, 32)), ldImg("imgs/fire.png")
fanOutImg.blit(ldImg("imgs/fan.png", 32), (0, 0))
for i in range(32):
  for j in range(32):
    if (j // 4) % 2: boosterImg.set_at((i, j), colorFromPalette((j // 8) % 2))
    else: boosterImg.set_at((i, j), colorFromPalette((((j // 8) % 2) + (i // 4)) % 2))
for i in range(32):
  for j in range(32):
    turboImg.set_at((i, j), colorFromPalette(((((j // 8) % 2) + (i // 4)) % 2) + 2))
firebarImg.blit(fireImg, (0, 0))
playerImg = pygame.image.load("imgs/player/player.png")
playerImg.blit(pygame.image.load("imgs/player/face.png"), (0, 0))
tiles = [wallImg, groundImg, waterImg, goalImg, fakeBarrierImg, boosterImg, fireSpitterImg, fireImg, enemyImg, fishImg, sharkImg, switchImg, sBlock1Img, sBlock2Img, sTimeImg, sTKillImg, fanUpImg, fanOutImg, turboImg, firebarImg]
terrain = None
layout = None
startPos = None
if saveMode == "json":
  try:
    with open("levels/%s/screen%i.json" % (levelName, screenNum)) as f: terrain = json.loads(f.read())
  except OSError:
    terrain = {
      "layout": [],
      "startpos": [4, 4]
    }
    for i in range(45):
      terrain["layout"].append([])
      for j in range(80): terrain["layout"][i].append(0)
    try: os.mkdir("levels/%s" % levelName)
    except FileExistsError: pass
    with open("levels/%s/screen%i.json" % (levelName, screenNum), "w") as f: f.write(lvlToJson(terrain["layout"], terrain["startpos"]))
  layout = terrain["layout"]
  startPos = terrain["startpos"]
elif saveMode == "lvl":
  try:
    with open("levels/%s/screen%i.lvl" % (levelName, screenNum), "rb") as f: layout, startPos = lvlToLvlIn(f.read())
  except OSError:
    layout = []
    startPos = [4, 4]
    for i in range(45):
      layout.append([])
      for j in range(80): layout[i].append(0)
    try: os.mkdir("levels/%s" % levelName)
    except FileExistsError: pass
    with open("levels/%s/screen%i.lvl" % (levelName, screenNum), "wb") as f: f.write(bytes(lvlToLvlOut(layout, startPos)))

try:
  while loop:
    clock.tick(30)
    timer += 1
    pygame.display.update()
    screen.fill((0, 0, 0))
    mX, mY = (pygame.mouse.get_pos()[0] + cam[0]) // 32, (pygame.mouse.get_pos()[1] + cam[1]) // 32
    for e in pygame.event.get():
      if e.type == pygame.QUIT: loop = 0
      if e.type == pygame.KEYDOWN:
        keys.append(e.key)
        for i in range(10): exec("if e.key == K_%i:\n if selectedTile %s 10 == %i: selectedTile += 10\n else: selectedTile = %i\n if selectedTile > maxTiles: selectedTile = %i" % (i, "%", i, i, i))
        if e.key == K_ESCAPE: loop = 0
        if e.key == K_e: print("(%i, %i)" % (mX, mY))
      if e.type == pygame.KEYUP:
        try: keys.remove(e.key)
        except: pass

    if K_w in keys: cam[1] -= 8
    if K_a in keys: cam[0] -= 8
    if K_s in keys: cam[1] += 8
    if K_d in keys: cam[0] += 8
    if mX > -1 and mX < 81 and mY > -1 and mY < 46:
      if pygame.mouse.get_pressed()[0]: layout[mY][mX] = selectedTile
      if pygame.mouse.get_pressed()[1]: selectedTile = layout[mY][mX]
      if pygame.mouse.get_pressed()[2]: startPos = [mX, mY]
    for i in range(45):
      for j in range(80):
        posX = j * 32 - cam[0]
        posY = i * 32 - cam[1]
        if posX < -32 or posX > 1280 or posY < -32 or posY > 720: continue
        if not ((mX, mY) == (j, i) and (timer // 8) % 2): screen.blit(tiles[layout[i][j]], (posX, posY, 32, 32))
        else: screen.blit(tiles[selectedTile], (posX, posY, 32, 32))
    pygame.draw.rect(screen, (0xa0, 0xff, 0x40) if (timer // 4) % 2 else (0x08, 0xa0, 0x00), (mX * 32 - cam[0], mY * 32 - cam[1], 32, 32), 2)
    screen.blit(playerImg, (startPos[0] * 32 - cam[0] + 13, startPos[1] * 32 - cam[1] + 13))
finally:
  if saveMode == "json":
    with open("levels/%s/screen%ibackup.json" % (levelName, screenNum), "w") as f:
      with open("levels/%s/screen%i.json" % (levelName, screenNum)) as f2: f.write(f2.read())
    with open("levels/%s/screen%i.json" % (levelName, screenNum), "w") as f: f.write(lvlToJson(layout, startPos))
  elif saveMode == "lvl":
    with open("levels/%s/screen%ibackup.lvl" % (levelName, screenNum), "wb") as f:
      with open("levels/%s/screen%i.lvl" % (levelName, screenNum), "rb") as f2: f.write(f2.read())
    with open("levels/%s/screen%i.lvl" % (levelName, screenNum), "wb") as f:
      try: f.write(bytes(lvlToLvlOut(layout, startPos)))
      except RuntimeError:
        with open("levels/%s/screen%i.lvl" % (levelName, screenNum), "rb") as f2: f.write(f2.read())
        print("Too many tiles!")
  pygame.quit()