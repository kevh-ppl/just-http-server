
CFLAGS := -Wall -Wextra -pedantic
CC := gcc
OBJECT_NAME := server

INCLUDE_DIR := include
CFILES := $(wildcard src/*.c)

$(OBJECT_NAME): $(CFILES)
	$(CC) $(CFLAGS) -o $@ -I$(INCLUDE_DIR) $(CFILES)
