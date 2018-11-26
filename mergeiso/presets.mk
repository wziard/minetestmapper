CPP_OBJECTS_BARE+=sdlmain
CXXFLAGS+=`sdl2-config --cflags`
LIBS+=`sdl2-config --libs` -lSDL2_image
