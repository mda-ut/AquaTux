SRCS:=$(wildcard *.c)
OBJ:=$(patsubst %.c,%.o,$(SRCS))
SO=libui.so
UI=ui
EXES=$(UI)

include ../Makefile.in
include Makefile.deps

MAIN_SRC=main_src/main.c

$(UI): $(MAIN_SRC)
	$(CC) -o $@ $(MAIN_SRC) $(CXXFLAGS) -I . $(CV_LIBS)

.PHONY: $(PHONY_TARGETS)
