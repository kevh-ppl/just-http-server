
CFLAGS := -Wall -Wextra -pedantic -g
CC := gcc
OBJECT_NAME := server

.PHONY: valgrid

INCLUDE_DIR := include
CFILES := $(wildcard src/*.c)

$(OBJECT_NAME): $(CFILES)
	$(CC) $(CFLAGS) -o $@ -I$(INCLUDE_DIR) $(CFILES)

valgrid: $(OBJECT_NAME)
	@valgrind --track-origins=yes --show-leak-kinds=all ./$(OBJECT_NAME)
