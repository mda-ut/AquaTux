SIM=sim
EXES=$(SIM)
SO=libsim.so

include ../Makefile.in
include Makefile.deps

SIM_SRC=main_src/main.cpp

$(SIM): $(SIM_SRC)
	$(CXX) -o $@ $(SIM_SRC) $(CXXFLAGS) -I . $(CV_LIBS)

.PHONY: $(PHONY_TARGETS)
