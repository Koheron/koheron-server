# Makefile for Koheron API core library
#
# Thomas Vanderbruggen <thomas@koheron.com>
# 02/05/2015
#
# (c) Koheron 2015

TARGET_OS=Linux
#TARGET_OS=Win32
#TARGET_OS=win64

BUILD=DEBUG_KOHERON_API
#BUILD=RELEASE_KOHERON_API

OBJS = kclient.o devmem.o

SRCS = $(subst .o,.c, $(OBJS), ))

CFLAGS= -fvisibility=hidden -DBUILDING_KOHERON_LIB -D$(BUILD) -Wall -Werror -std=gnu99 -O3
#GDBFLAGS = -g3 -O0 -ggdb
#CFLAGS += $(GDBFLAGS)

LDFLAGS= -shared

ifeq ($(TARGET_OS),Linux)
TOOLCHAIN=
CFLAGS += -fPIC
TARGET=libkoheron.so
else ifeq ($(TARGET_OS),Win32)
TOOLCHAIN=i686-w64-mingw32-
TARGET=libkoheron.dll
LDFLAGS += -lws2_32
else ifeq ($(TARGET_OS),Win64)
TOOLCHAIN=x86_64-w64-mingw32-
TARGET=libkoheron.dll
endif

CC=$(TOOLCHAIN)gcc

all: $(TARGET)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@
	
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	
install: $(TARGET)
	cp $(TARGET) ./koheron
	
clean:
	rm -f $(TARGET) *.o $(OBJS)
	rm -f ./koheron/$(TARGET)
	
