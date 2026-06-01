#!/usr/bin/make -f

NAME = dih-stortion
CXXFLAGS += -std=gnu++20

FILES_DSP = \
	src/main.cpp

FILES_UI  = \
	src/ui.cpp \
	src/ArrowArtwork.cpp \
	src/BackgroundArtwork.cpp

include dpf/Makefile.plugins.mk

TARGETS += jack
# TARGETS += clap
# TARGETS += lv2_sep
# TARGETS += vst2
TARGETS += vst3

all: $(TARGETS)

.PHONY: all
