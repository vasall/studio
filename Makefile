CC := gcc

CFLAGS := \
	-Wall \
	-Wextra \
	-pedantic \
	-std=c99 \
	-D_POSIX_C_SOURCE=200809L \
	-g \
	-O1 \
	-MMD \
	-MP

DEVKIT_DIR := ../devkit
DEVKIT_LIB := $(DEVKIT_DIR)/build/libvasall.so

SDL_CFLAGS := $(shell pkg-config --cflags sdl3)
SDL_LIBS := $(shell pkg-config --libs sdl3)

INCLUDES := \
	-Iinclude \
	-I$(DEVKIT_DIR)/include \
	-Iexternal/glad/include \
	-Iexternal/stb \
	$(SDL_CFLAGS)

# The binary finds libvasall.so relative to itself: build/ -> ../../devkit/build
LIBS := \
	-L$(DEVKIT_DIR)/build \
	-lvasall \
	-Wl,-rpath,'$$ORIGIN/../$(DEVKIT_DIR)/build' \
	$(SDL_LIBS) \
	-lm

BUILD_DIR := build
BIN := $(BUILD_DIR)/vasall-studio

UNITY_DIR := $(DEVKIT_DIR)/third_party/unity/src

#
# Sources
#

SRC := $(shell find source -type f -name '*.c')
OBJ := $(patsubst source/%.c,$(BUILD_DIR)/obj/%.o,$(SRC))

# Third-party code, built without our warnings
EXT_SRC := external/glad/src/gl.c
EXT_OBJ := $(patsubst external/%.c,$(BUILD_DIR)/ext/%.o,$(EXT_SRC))

# Objects that don't need a window or GL, so tests can link them
HEADLESS_OBJ := $(filter $(BUILD_DIR)/obj/core/% $(BUILD_DIR)/obj/ui/%,$(OBJ)) \
	$(BUILD_DIR)/obj/workbench/dock.o \
	$(BUILD_DIR)/obj/workbench/view.o \
	$(BUILD_DIR)/obj/workbench/dummy_view.o

TEST_SRC := $(shell find tests -type f -name '*.c' 2>/dev/null)
TEST_BIN := $(patsubst tests/%.c,$(BUILD_DIR)/tests/%,$(TEST_SRC))

#
# Targets
#

.PHONY: all
all: $(BIN) $(TEST_BIN)

.PHONY: devkit
devkit:
	$(MAKE) -C $(DEVKIT_DIR) build/libvasall.so

$(DEVKIT_LIB): devkit

$(BIN): $(OBJ) $(EXT_OBJ) | devkit
	@mkdir -p $(dir $@)
	$(CC) -o $@ $(OBJ) $(EXT_OBJ) $(LIBS)

$(BUILD_DIR)/obj/%.o: source/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# stb implementations: third-party code, no warnings
$(BUILD_DIR)/obj/render/stb_impl.o: source/render/stb_impl.c
	@mkdir -p $(dir $@)
	$(CC) -std=c99 -g -O2 -w $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/ext/%.o: external/%.c
	@mkdir -p $(dir $@)
	$(CC) -std=c99 -g -O2 -w $(INCLUDES) -c $< -o $@

#
# Tests
#

$(BUILD_DIR)/tests/%: tests/%.c $(HEADLESS_OBJ) | devkit
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -I$(UNITY_DIR) \
		$< $(HEADLESS_OBJ) $(UNITY_DIR)/unity.c \
		-L$(DEVKIT_DIR)/build -lvasall \
		-Wl,-rpath,'$$ORIGIN/../../../$(DEVKIT_DIR)/build' -lm \
		-o $@

.PHONY: test
test: $(TEST_BIN)
	@for test in $(TEST_BIN); do \
		echo ""; \
		echo "========================================"; \
		echo "Running $$test"; \
		echo "========================================"; \
		$$test || exit 1; \
	done

.PHONY: run
run: $(BIN)
	./$(BIN)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

-include $(OBJ:.o=.d)
