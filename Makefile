#!/usr/bin/make -f

NAME = mute

FILES_DSP = \
	src/main.cpp

include dpf/Makefile.plugins.mk

TARGETS += clap
TARGETS += jack
TARGETS += lv2_sep
TARGETS += vst2
TARGETS += vst3

all: $(TARGETS)

.PHONY: all
