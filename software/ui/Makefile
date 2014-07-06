SRCS:=$(wildcard *.c)
OBJ:=$(patsubst %.c,%.o,$(SRCS))
A=libui.a
UI=ui
EXES=$(UI)

include ../Makefile.in
include Makefile.deps

MAIN_SRC=main_src/main.c

$(UI): $(OBJ) $(MAIN_SRC)
	$(CC) -o $@ $(MAIN_SRC) $(OBJ) $(CXXFLAGS) -I .

.PHONY: $(PHONY_TARGETS)
