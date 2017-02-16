# Project directories
SRC_DIR = ./src
INC_DIR = ./include
OBJ_DIR = ./obj

# Compilation options
CC = gcc
CFLAGS = -I$(INC_DIR) -Wextra -Wall
LDFLAGS = -lm -lpthread

# List of object file
OBJ_FILES = $(OBJ_DIR)/csapp.o $(OBJ_DIR)/socket_helper.o \
$(OBJ_DIR)/packet_helper.o $(OBJ_DIR)/packet_handler.o \
$(OBJ_DIR)/file_handler.o $(OBJ_DIR)/hash.o $(OBJ_DIR)/tasks.o \
$(OBJ_DIR)/get_options.o $(OBJ_DIR)/hash2.o

################################################################################

all : child super

# Generation of main object file for child program
$(OBJ_DIR)/child.o: $(SRC_DIR)/child.c
	$(CC) -c $(CFLAGS) $< -o $@ $(LDFLAGS)

# Generation of main object file for super program
$(OBJ_DIR)/super.o: $(SRC_DIR)/super.c
	$(CC) -c $(CFLAGS) $< -o $@ $(LDFLAGS)

# Generation of an object file from a source file and its specification file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/%.h
	$(CC) -c $(CFLAGS) $< -o $@ $(LDFLAGS)

# Linking of all object files for child program
child: $(OBJ_DIR)/child.o $(OBJ_FILES)
	$(CC) -o $@ $^ $(LDFLAGS)

# Linking of all object files for super program
super: $(OBJ_DIR)/super.o $(OBJ_FILES)
	$(CC) -o $@ $^ $(LDFLAGS)

################################################################################

# Clean Up
.PHONY: clean
clean: clean_temp
	rm -f super child $(OBJ_DIR)/*.o source.tar.gz

.PHONY: clean_temp
clean_temp:
	rm -f ./*/*~
	rm -f ./*~

################################################################################

.PHONY: compress
compress: clean
	tar -czvf source.tar.gz include/ obj/ src/ Makefile README.md

################################################################################

# Help
.PHONY: help
help:
	@echo "Options :-"
	@echo "1) make / make all - to compile everything"
	@echo "2) make clean - clean up the object files and executables"
	@echo "3) make clean_temp - clean up temporary files"
	@echo "4) make compress - compress the source code, readme and makefile"
	@echo "5) make help - help menu"
