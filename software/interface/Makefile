AQUATUX=aquatux
EXES=$(AQUATUX)
SO=libinterface.so

include ../Makefile.in
include Makefile.deps

AQUATUX_SRC=main_src/main.cpp

$(AQUATUX): $(AQUATUX_SRC)
	$(CXX) -o $@ $(AQUATUX_SRC) $(CXXFLAGS) -I .

.PHONY: $(PHONY_TARGETS)
