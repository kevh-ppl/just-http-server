
CFLAGS := -Wall -Wextra -pedantic -g
CC := gcc
OBJECT_NAME := server

.PHONY: valgrid compile_commands.json

INCLUDE_DIR := include
CFILES := $(wildcard src/*.c)

$(OBJECT_NAME): $(CFILES)
	$(CC) $(CFLAGS) -o $@ -I$(INCLUDE_DIR) $(CFILES)

valgrid: $(OBJECT_NAME)
	@valgrind --track-origins=yes --show-leak-kinds=all ./$(OBJECT_NAME)

# Base de datos de compilacion para clangd (LSP). Regenerar al anadir fuentes.
compile_commands.json:
	@printf '[\n' > $@
	@first=1; for f in $(CFILES); do \
		[ $$first -eq 1 ] || printf ',\n' >> $@; \
		printf '  {"directory": "%s", "file": "%s", "command": "%s %s -I%s -c %s -o /dev/null"}' \
			"$(CURDIR)" "$$f" "$(CC)" "$(CFLAGS)" "$(INCLUDE_DIR)" "$$f" >> $@; \
		first=0; \
	done
	@printf '\n]\n' >> $@
	@echo "compile_commands.json generado para $(words $(CFILES)) fuentes"
