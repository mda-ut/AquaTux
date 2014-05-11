MAIN=main
WEBCAM=webcam
HU=hu
EXES=$(MAIN) $(WEBCAM) $(HU)
SO=libvision.so

include ../Makefile.in
include Makefile.deps

MAIN_DIR=main_src/
MAIN_SRC=$(MAIN_DIR)main.cpp
WEBCAM_SRC=$(MAIN_DIR)webcam.cpp
HU_SRC=$(MAIN_DIR)hu.cpp

$(MAIN): $(MAIN_SRC)
	$(CXX) -o $@ $(MAIN_SRC) $(CXXFLAGS) -I .

$(WEBCAM): $(WEBCAM_SRC)
	$(CXX) -o $@ $(WEBCAM_SRC) $(CXXFLAGS) -I .

$(HU): $(HU_SRC)
	$(CXX) -o $@ $(HU_SRC) $(CXXFLAGS) -I .

.PHONY: $(PHONY_TARGETS)
