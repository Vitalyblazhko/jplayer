UBUNTU_REL = $(shell lsb_release -rs)

ifeq ($(UBUNTU_REL),20.04)
	GTK_VER ?= gtk+-3.0
else
	GTK_VER ?= gtk+-2.0
endif

CC = g++
SRC = jplayer.cpp window_util.cpp
OBJ = $(SRC:.cpp = .o)


jplayer: $(OBJ)
	$(CC) -g -o jplayer $(OBJ) -lX11 -lopencv_world `pkg-config --libs --cflags $(GTK_VER)`

clean:
	rm -f core *.o jplayer
