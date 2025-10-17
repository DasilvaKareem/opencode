CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I/opt/homebrew/include
LDFLAGS = -lraylib -lm -L/opt/homebrew/lib

ifeq ($(shell uname), Darwin)
    LDFLAGS += -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
else
    LDFLAGS += -lpthread -ldl -lrt -lX11
endif

EXAMPLES = examples/basic_window examples/player_movement_2d examples/basic_3d examples/3d_sailor

all: $(EXAMPLES)

examples/%: examples/%.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(EXAMPLES)

.PHONY: all clean
