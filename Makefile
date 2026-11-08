CC = gcc

DEVKIT_DIR = ../devkit
DEVKIT_LIB = $(DEVKIT_DIR)/build/libvasall.so

STUDIO_DIR = .

CFLAGS = \
	-std=c99 \
	-Wall \
	-Wextra \
	-I$(STUDIO_DIR)/include \
	-Iexternal/glad/include \
	-Iexternal/stb \
	-I$(DEVKIT_DIR)/include \
	$(shell pkg-config --cflags sdl3)

LDFLAGS = \
	-L$(DEVKIT_DIR)/build \
	-lvasall \
	-Wl,-rpath,'$$ORIGIN/$(DEVKIT_DIR)/build' \
	$(shell pkg-config --libs sdl3) \
	-lGL \
	-lm

SRC = \
	source/main.c \
	$(shell find module -type f -name '*.c') \
	external/glad/src/gl.c

studio: $(SRC) $(DEVKIT_LIB)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o studio

# Build the devkit library if it is missing or out of date
.PHONY: devkit
$(DEVKIT_LIB): devkit
devkit:
	$(MAKE) -C $(DEVKIT_DIR) build/libvasall.so

.PHONY: clean
clean:
	rm -f studio
