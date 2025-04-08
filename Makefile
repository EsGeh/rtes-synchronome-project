
#########################
# Variables
#########################

SRC_DIR=src
BUILD_DIR=build
CONFIG=debug

CC=gcc
CFLAGS=-O0 -Wall -Wextra -g
OUT_DIR=$(BUILD_DIR)/$(CONFIG)
OBJ_DIR=$(OUT_DIR)/objs


#########################
# Recipies
#########################

.PHONY: clean

all: $(OUT_DIR)/test_performance

clean:
	rm -rf $(OUT_DIR)

$(OUT_DIR)/test_performance: $(OBJ_DIR)/test_performance.o | init_dirs
	$(CC) $(CFLAGS) -o $@ $?

$(OBJ_DIR)/test_performance.o: $(SRC_DIR)/test_performance.c | init_dirs
	$(CC) $(CFLAGS) -o $@ -c $?

init_dirs: $(OUT_DIR) $(OBJ_DIR)

$(OUT_DIR):
	mkdir --parents $(OUT_DIR)

$(OBJ_DIR):
	mkdir --parents $(OBJ_DIR)
