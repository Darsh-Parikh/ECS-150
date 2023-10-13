EXTENSION = 

CC = gcc
CFLAGS = -Wall -Werror -g

SRC_DIR = wcat wgrep wzip wunzip

SOURCES := $(foreach dir, $(SRC_DIR), $(wildcard $(dir)/*.c))
OBJECTS := $(SOURCES:.c=$(EXTENSION))

all: $(OBJECTS)

%$(EXTENSION) : %.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm $(OBJECTS)
