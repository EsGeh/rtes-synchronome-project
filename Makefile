
#########################
# Variables
#########################

SRC_DIR=src
TEST_DIR=src/tests
BUILD_DIR=build
CONFIG=debug

CC=gcc
ifeq (${CONFIG},debug)
CFLAGS=-O0 -Wall -Wextra -pedantic -g \
 -fprofile-arcs -ftest-coverage
else
CFLAGS=-O3 -Wall -Wextra -pedantic
endif
OUT_DIR=$(BUILD_DIR)/$(CONFIG)
OBJ_DIR=$(OUT_DIR)/objs

#########################
# Recipies
#########################

.PHONY: clean

all: \
	$(OUT_DIR)/synchronome \
	$(OUT_DIR)/run_tests \
	$(OUT_DIR)/statistics

clean:
	rm -rf $(OUT_DIR)

$(OUT_DIR)/synchronome: \
		$(OBJ_DIR)/synchronome.o \
		$(OBJ_DIR)/camera.o \
		$(OBJ_DIR)/image.o \
		$(OBJ_DIR)/output.o \
		| init_dirs
	$(CC) $(CFLAGS) -o $@ $^

$(OUT_DIR)/statistics: \
		$(OBJ_DIR)/statistics.o \
		$(OBJ_DIR)/camera.o \
		$(OBJ_DIR)/image.o \
		$(OBJ_DIR)/time.o \
		$(OBJ_DIR)/output.o \
		| init_dirs
	$(CC) $(CFLAGS) -o $@ $^

$(OUT_DIR)/run_tests: \
		$(OBJ_DIR)/run_tests.o \
		$(OBJ_DIR)/camera.o \
		$(OBJ_DIR)/image.o \
		$(OBJ_DIR)/time.o \
		$(OBJ_DIR)/output.o \
		| init_dirs
	$(CC) $(CFLAGS) -o $@ $^ `pkg-config --cflags --libs check`

$(OBJ_DIR)/synchronome.o: \
		$(SRC_DIR)/exe/synchronome.c \
		$(SRC_DIR)/lib/camera.h \
		$(SRC_DIR)/lib/image.h \
		$(SRC_DIR)/lib/output.h \
		| init_dirs
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c -o $@ $<

$(OBJ_DIR)/statistics.o: \
		$(SRC_DIR)/exe/statistics.c \
		$(SRC_DIR)/lib/camera.h \
		$(SRC_DIR)/lib/time.h \
		$(SRC_DIR)/lib/output.h \
		| init_dirs
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c -o $@ $<

$(OBJ_DIR)/run_tests.o: \
		$(SRC_DIR)/exe/run_tests.c \
		$(TEST_DIR)/test_camera.c \
		$(SRC_DIR)/lib/camera.h \
		$(SRC_DIR)/lib/image.h \
		$(SRC_DIR)/lib/time.h \
		$(SRC_DIR)/lib/output.h \
		| init_dirs
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c -o $@ $<

# library / units:

$(OBJ_DIR)/camera.o: \
		$(SRC_DIR)/lib/camera.c $(SRC_DIR)/lib/camera.h \
		$(SRC_DIR)/lib/output.h \
		$(SRC_DIR)/lib/global.h \
		| init_dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/image.o: \
		$(SRC_DIR)/lib/image.c $(SRC_DIR)/lib/image.h \
		$(SRC_DIR)/lib/output.h \
		$(SRC_DIR)/lib/global.h \
		| init_dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/output.o: \
		$(SRC_DIR)/lib/output.c $(SRC_DIR)/lib/output.h \
		$(SRC_DIR)/lib/global.h \
		| init_dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/time.o: \
		$(SRC_DIR)/lib/time.c $(SRC_DIR)/lib/time.h \
		$(SRC_DIR)/lib/global.h \
		| init_dirs
	$(CC) $(CFLAGS) -c -o $@ $<

init_dirs: $(OUT_DIR) $(OBJ_DIR)

$(OUT_DIR):
	mkdir --parents $(OUT_DIR)

$(OBJ_DIR):
	mkdir --parents $(OBJ_DIR)
