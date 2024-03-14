CC = clang
C_FLAGS = -O0 -g -MMD -MP
L_FLAGS = -lm -lglfw -lGL -lGLEW

BIN = doom
BUILD_DIR = ./build

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:%.o=%.d)

ARGS = 

all: run

run: $(BIN)
	./$(BIN) $(ARGS)

$(BIN): $(OBJS)
	$(CC) $^ -o $@ $(L_FLAGS)

-include $(DEPS)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(C_FLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY : all run clean
