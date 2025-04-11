
#########################
# Variables
#########################

SRC_DIR=src
TEST_DIR=tests
BUILD_DIR=build
CONFIG=debug

CC=gcc
CFLAGS=-O0 -Wall -Wextra -pedantic -g \
 -fprofile-arcs -ftest-coverage
OUT_DIR=$(BUILD_DIR)/$(CONFIG)
OBJ_DIR=$(OUT_DIR)/objs

#########################
# Recipies
#########################

.PHONY: clean

all: $(OUT_DIR)/run_tests $(OUT_DIR)/platform_info

clean:
	rm -rf $(OUT_DIR)

$(OUT_DIR)/platform_info: \
		$(OBJ_DIR)/platform_info.o \
		$(OBJ_DIR)/camera.o \
		$(OBJ_DIR)/output.o \
		| init_dirs
	$(CC) $(CFLAGS) -o $@ $^

$(OUT_DIR)/run_tests: \
		$(OBJ_DIR)/run_tests.o \
		$(OBJ_DIR)/camera.o \
		$(OBJ_DIR)/time.o \
		$(OBJ_DIR)/output.o \
		| init_dirs
	$(CC) $(CFLAGS) -lcheck -o $@ $^

$(OBJ_DIR)/platform_info.o: \
		$(SRC_DIR)/platform_info.c \
		$(SRC_DIR)/camera.h \
		$(SRC_DIR)/output.h \
		| init_dirs
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c -o $@ $<

$(OBJ_DIR)/run_tests.o: \
		$(TEST_DIR)/run_tests.c \
		$(TEST_DIR)/test_camera.c \
		$(SRC_DIR)/camera.h \
		$(SRC_DIR)/time.h \
		$(SRC_DIR)/output.h \
		| init_dirs
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c -o $@ $<

# library / units:

$(OBJ_DIR)/camera.o: \
		$(SRC_DIR)/camera.c $(SRC_DIR)/camera.h \
		$(SRC_DIR)/output.h \
		$(SRC_DIR)/global.h \
		| init_dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/output.o: \
		$(SRC_DIR)/output.c $(SRC_DIR)/output.h \
		$(SRC_DIR)/global.h \
		| init_dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/time.o: \
		$(SRC_DIR)/time.c $(SRC_DIR)/time.h \
		$(SRC_DIR)/global.h \
		| init_dirs
	$(CC) $(CFLAGS) -c -o $@ $<

init_dirs: $(OUT_DIR) $(OBJ_DIR)

$(OUT_DIR):
	mkdir --parents $(OUT_DIR)

$(OBJ_DIR):
	mkdir --parents $(OBJ_DIR)
