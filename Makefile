CC := gcc
CFLAGS := -O2 -Wall -Wextra -std=c11
LDFLAGS := -lm
LDLIBS := -lgd -lm
SOLVER_BIN := wordsearch_solver
TRAIN_BIN := ocr_nn
PRETREATMENT_BIN := pretreatment
OCR_COMMON_SRCS := \
	src/nn.c \
	src/nn_io.c \
	src/train_image_loader.c \
	src/image_loader.c \
	src/image_utils.c \
	src/stb_impl.c \
	solver/solver1.c
OCR_COMMON_OBJS := $(OCR_COMMON_SRCS:.c=.o)
OCR_PRETREATMENT_SRCS := \
	src/image_loader.c \
	src/image_utils.c \
	src/noise_removal.c \
	src/rotation.c
OCR_PRETREATMENT_OBJS := $(OCR_PRETREATMENT_SRCS:.c=.o)
OCR_TRAIN_SRCS := \
	src/dataset.c \
	src/ocr_main.c
OCR_TRAIN_OBJS := $(OCR_TRAIN_SRCS:.c=.o)
OCR_PROD_SRCS := \
	src/ocr_prod.c
OCR_PROD_OBJS := $(OCR_PROD_SRCS:.c=.o)
LOC_SRCS := \
	localization/projection.c \
	localization/grid_detection.c \
	localization/letter_extraction.c \
	localization/wordlist_detection.c \
	localization/wordlist_extraction.c
LOC_OBJS := $(LOC_SRCS:.c=.o)
MAIN_SRC := localization/main_with_wordlist.c
MAIN_OBJ := $(MAIN_SRC:.c=.o)
PMAIN_SRC := src/main.c
PMAIN_OBJ := $(PMAIN_SRC:.c=.o)
all: $(SOLVER_BIN) $(TRAIN_BIN) $(PRETREATMENT_BIN)
$(SOLVER_BIN): $(MAIN_OBJ) $(LOC_OBJS) $(OCR_COMMON_OBJS) $(OCR_PROD_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
$(TRAIN_BIN): $(OCR_COMMON_OBJS) $(OCR_TRAIN_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
$(PRETREATMENT_BIN): $(PMAIN_OBJ) $(OCR_PRETREATMENT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f *.o $(SOLVER_BIN) $(TRAIN_BIN) $(PRETREATMENT_BIN)
	rm -f grid.txt words.txt
	rm -rf data/grid/cells/*
	rm -rf data/wordlist/cells/*
	rm -rf data/rotation_test/*
	rm -rf data/images/*
	rm -rf data/debug/*
	rm -f localization_debug.png
	rm -f coords.txt
	rm -rf src/data/debug/*
	rm -rf src/data/images/*
	rm -rf src/data/rotation_test/*
.PHONY: all clean