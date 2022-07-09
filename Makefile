# Makefile for Basler pylon sample program
.PHONY: all clean

# The program to build
NAME       := Grab

# Installation directories for pylon
PYLON_ROOT ?= /opt/pylon

# Build tools and flags
LD         := $(CXX) -std=c++17
CPPFLAGS   := $(shell $(PYLON_ROOT)/bin/pylon-config --cflags) $(shell pkg-config opencv4 --cflags) $(shell pkg-config boost --cflags)
CXXFLAGS   := -std=c++17 #e.g., CXXFLAGS=-g -O0 for debugging
LDFLAGS    := $(shell $(PYLON_ROOT)/bin/pylon-config --libs-rpath)
LDLIBS     := $(shell $(PYLON_ROOT)/bin/pylon-config --libs)  $(shell pkg-config opencv4 --libs) $(shell pkg-config boost179 --libs) -lpthread

OUTPUT := imageprocessor
BOOST = -lboost_system -lboost_filesystem

# Rules for building
all: $(NAME)

$(NAME): $(NAME).o imageprocessor.o $(BOOST)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS) 
	
$(OUTPUT).o: $(OUTPUT).cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(NAME).o: $(NAME).cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	$(RM) $(NAME).o $(NAME)
