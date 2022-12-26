CFLAGS         = `pkg-config --libs --cflags sdl2 sdl2_image sdl2_ttf`

all: game

game: sdl_tutorial.c
	clang -g -O0 sdl_tutorial.c -o game $(CFLAGS)

# clang -g sdl_tutorial.c -o game `pkg-config --libs --cflags sdl2 sdl2_image`