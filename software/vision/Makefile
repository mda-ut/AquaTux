MAIN=main
WEBCAM=webcam
EXES=$(MAIN) $(WEBCAM)
A=libvision.a

include ../Makefile.in
include Makefile.deps

MAIN_DIR=main_src/
MAIN_SRC=$(MAIN_DIR)main.cpp
WEBCAM_SRC=$(MAIN_DIR)webcam.cpp

$(MAIN): $(OBJ) $(MAIN_SRC) $(LIB)
	$(CXX) -o $@ $(OBJ) $(MAIN_SRC) $(CXXFLAGS) -I . $(CV_LIBS)

$(WEBCAM): $(OBJ) $(WEBCAM_SRC) $(LIB)
	$(CXX) -o $@ $(OBJ) $(WEBCAM_SRC) $(CXXFLAGS) -I . $(CV_LIBS)

.PHONY: $(PHONY_TARGETS)
