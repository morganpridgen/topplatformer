FILES = main.cpp window.cpp helpers.cpp player.cpp controller.cpp terrain.cpp enemy.cpp
COMPILER = g++
FLAGS = -w
DEBUGFLAGS = -Wall -ggdb -pg -no-pie
SPEEDFLAGS = -O2
LINKED = -lSDL2 -lSDL2_image -lSDL2_mixer
WINFLAGS = -w
WININCLUDES = -IC:\SDL2\include
WINLIB = -LC:\SDL2\lib
WINLINKED = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer
OUT = topplatformer

all: $(FILES)
	$(COMPILER) $(FILES) $(FLAGS) $(LINKED) -o $(OUT)
debug: $(FILES)
	$(COMPILER) $(FILES) $(DEBUGFLAGS) $(LINKED) -o $(OUT)
speed: $(FILES)
	$(COMPILER) $(FILES) $(SPEEDFLAGS) $(LINKED) -o $(OUT)
win: $(FILES)
	$(COMPILER) $(FILES) winstuff.res $(WININCLUDES) $(WINLIB) $(WINFLAGS) $(WINLINKED) -o $(OUT)