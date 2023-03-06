# excerpt from ./extern/whispercpp/Makefile
SHELL := /bin/bash
CXX := clang

ifndef UNAME_S
UNAME_S := $(shell uname -s)
endif

CFLAGS   = -O3 -fPIC -std=c11   $(shell python3-config --includes) -Iextern -I./extern/pybind11/include -DGGML_USE_ACCELERATE
CXXFLAGS = -O3 -fPIC -std=c++11 $(shell python3-config --includes) -Iextern -I./extern/pybind11/include -I./extern/whispercpp/examples
LDFLAGS  =

ifeq ($(UNAME_S),Linux)
	CFLAGS   += -pthread
	CXXFLAGS += -pthread
endif
ifeq ($(UNAME_S),Darwin)
	CFLAGS   += -pthread
	CXXFLAGS += -pthread
endif

default: api

EXTRA_CXXFLAGS = -Isrc/whispercpp/
ifeq ($(UNAME_S),Darwin)
	EXTRA_CXXFLAGS += -Wl,-undefined,dynamic_lookup
endif

context.o: src/whispercpp/context.cc src/whispercpp/context.h
	$(CXX) $(CXXFLAGS) -o src/whispercpp/context.o -c src/whispercpp/context.cc

api_cpp2py_export.o: src/whispercpp/api_cpp2py_export.cc
	$(CXX) $(CXXFLAGS) -o src/whispercpp/api_cpp2py_export.o -c src/whispercpp/api_cpp2py_export.cc

api: api_cpp2py_export.o context.o
	@echo "Building pybind11 extension..."
	@cd ./extern/whispercpp && $(MAKE) ggml.o whisper.o && mv whisper.o ggml.o ../../src/whispercpp/
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) -shared -o src/whispercpp/api_cpp2py_export.so src/whispercpp/*.o

clean:
	rm -rf **/*.o **/*.so
