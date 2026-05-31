#!/usr/bin/make -f

NAME = mute

FILES_DSP = \
	src/main.cpp

FILES_UI  = \
	src/ui.cpp

include dpf/Makefile.plugins.mk

TARGETS += jack
# TARGETS += clap
# TARGETS += lv2_sep
# TARGETS += vst2
TARGETS += vst3

all: $(TARGETS)

.PHONY: all
