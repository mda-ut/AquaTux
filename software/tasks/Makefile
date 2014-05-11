MAIN=main
EXES=$(MAIN)
SO=libtasks.so

include ../Makefile.in
include Makefile.deps

MAIN_SRC=main_src/main.cpp

$(MAIN): $(MAIN_SRC)
	$(CXX) -o $@ $(MAIN_SRC) $(CXXFLAGS) -I .

.PHONY: $(PHONY_TARGETS)
