DEVICES:= tests/tests.hpp tests/tests.cpp

CROSS_COMPILE:=
ARCH_FLAGS:= -march=native

_VPATH=tests

#CROSS_COMPILE:=/usr/bin/arm-linux-gnueabihf-
#ARCH_FLAGS:= -march=armv7-a -mtune=cortex-a9 -mfpu=vfpv3 -mfpu=neon -mfloat-abi=hard

# Use Link Time Optimization
CCXX=$(CROSS_COMPILE)g++ -flto