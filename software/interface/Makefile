AQUATUX=aquatux
EXES=$(AQUATUX)

include ../Makefile.in
include Makefile.deps

AQUATUX_SRC=main_src/main.cpp

$(AQUATUX): $(OBJ) $(LIB) $(AQUATUX_SRC)
	$(CXX) -o $@ $(AQUATUX_SRC) $(OBJ) $(LIB) $(CXXFLAGS) $(CV_LIBS)

.PHONY: $(PHONY_TARGETS)
