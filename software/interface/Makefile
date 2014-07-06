AQUATUX=aquatux
EXES=$(AQUATUX)

include ../Makefile.in
include Makefile.deps

$(AQUATUX): $(OBJ) $(LIB)
	$(CXX) -o $@ $(OBJ) $(LIB) $(CXXFLAGS) $(CV_LIBS)

.PHONY: $(PHONY_TARGETS)
