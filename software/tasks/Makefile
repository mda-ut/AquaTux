MAIN=main
EXES=$(MAIN)
SO=libtasks.so
DEP_FLAGS=$(LIBVISION)

include ../Makefile.in
include Makefile.deps

MAIN_SRC=main_src/main.cpp

$(MAIN): $(MAIN_SRC)
	$(CXX) -o $@ $(MAIN_SRC) $(CXXFLAGS) -I . $(CV_LIBS)

.PHONY: $(PHONY_TARGETS)
