MAIN=main
WEBCAM=webcam
EXES=$(MAIN) $(WEBCAM)
SO=libvision.so

include ../Makefile.in
include Makefile.deps

MAIN_DIR=main_src/
MAIN_SRC=$(MAIN_DIR)main.cpp
WEBCAM_SRC=$(MAIN_DIR)webcam.cpp

$(MAIN): $(MAIN_SRC)
	$(CXX) -o $@ $(MAIN_SRC) $(CXXFLAGS) -I . $(CV_LIBS)

$(WEBCAM): $(WEBCAM_SRC)
	$(CXX) -o $@ $(WEBCAM_SRC) $(CXXFLAGS) -I . $(CV_LIBS)

.PHONY: $(PHONY_TARGETS)
