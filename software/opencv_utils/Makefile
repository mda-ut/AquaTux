SRCS:=$(wildcard *.cpp)
OBJ:=$(patsubst %.cpp,%.o,$(SRCS))
EXES:=$(patsubst %.cpp,%.out,$(SRCS))

include ../Makefile.in
include Makefile.deps

%.out: %.o $(LIB)
	$(CXX) -o $@ $< $(CXXFLAGS)

.PHONY: parallel all deps clean
