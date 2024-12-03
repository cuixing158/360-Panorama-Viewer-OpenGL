all:
	g++ -I/usr/include/SDL2/ -L/usr/lib/i386-linux-gnu/ -O2 main.cpp glrenderer.cpp -lSDL2 -lSDL2_image -lGL -lGLU -o program
debug:
	g++ -I/usr/include/SDL2/ -L/usr/lib/i386-linux-gnu/ -g -O0 main.cpp glrenderer.cpp -lSDL2 -lSDL2_image -lGL -lGLU -o program_dbg
