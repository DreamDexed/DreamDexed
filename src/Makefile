#
# Makefile
#

CIRCLE_STDLIB_DIR = ../circle-stdlib
SYNTH_DEXED_DIR = ../Synth_Dexed/src
CMSIS_DIR = ../CMSIS_5/CMSIS

OBJS = main.o kernel.o minidexed.o config.o userinterface.o uimenu.o ddfont8x16.o ddfont12x22.o \
       mididevice.o midikeyboard.o serialmididevice.o pckeyboard.o \
       sysexfileloader.o performanceconfig.o perftimer.o \
       effect.o effect_cloudseed2.o effect_platervbstereo.o effect_dreamdelay.o uibuttons.o midipin.o \
       ../CloudSeedCore/DSP/Biquad.o ../CloudSeedCore/DSP/RandomBuffer.o ../CloudSeedCore/DSP/FastSin.o \
       zyn/EffectLFO.o zyn/Phaser.o zyn/APhaser.o zyn/Chorus.o \
       zyn/AnalogFilter.o zyn/ValueSmoothingFilter.o zyn/WaveShapeSmps.o zyn/Distortion.o \
       zyn/CombFilterBank.o zyn/Sympathetic.o \
       butter.o \
       arm_float_to_q23.o arm_zip_f32.o arm_scale_zip_f32.o \
       net/ftpdaemon.o net/ftpworker.o net/applemidi.o net/udpmidi.o net/mdnspublisher.o udpmididevice.o \
       dawcontroller.o

EXTRACLEAN = $(OBJS) $(OBJS:.o=.d)

OPTIMIZE = -O3

DATE := $(shell git log -1 --format=%cd --date=format:%y%m%d)
COMMIT := $(shell git rev-parse --short HEAD)
VERSION := $(DATE)-$(COMMIT)
DEFINE += -DVERSION=\"$(VERSION)\"

include ./Synth_Dexed.mk
include ./Rules.mk
