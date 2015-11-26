# Makefile for KServer CLI
#
# @author Thomas Vanderbruggen <thomas@koheron.com>
# @date 19/08/2015
#
# (c) Koheron, 2014-2015

#TARGET_HOST = redpitaya
#TARGET_HOST = local

API_INC_PATH = ../api_c

# Toolchain
ifeq ($(TARGET_HOST),redpitaya)
#CROSS_COMPILE?=arm-linux-gnueabi-
CROSS_COMPILE?=arm-linux-gnueabihf-
#CROSS_COMPILE?=arm-xilinx-linux-gnueabi-
DEFINES += -DREDPITAYA
else ifeq ($(TARGET_HOST),local)
CROSS_COMPILE?=
DEFINES += -DLOCAL
endif

# Objects
OBJS_API = $(API_INC_PATH)/kclient.o  \
           $(API_INC_PATH)/sessions.o

OBJS = $(OBJS_API) config_file.o kserver_cli.o	
# List of raw source files (all object files, renamed from .o to .c)
SRCS = $(subst .o,.c, $(OBJS), ))

# Executable name
TARGET=kserver

# GCC compiling & linking flags
CFLAGS= -Wall -Werror -I$(API_INC_PATH) $(DEFINES) -g # -O3 # -g

ifeq ($(TARGET_HOST),redpitaya)
ARM_FLAGS = -march=armv7-a -mtune=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard
CFLAGS += $(ARM_FLAGS)
else ifeq ($(TARGET_HOST),local)
CFLAGS += -march=native
endif

# Libraries
LIBS = -lm # -lpthread -lssl -lcrypto

# Main GCC executable (used for compiling and linking)
CC=$(CROSS_COMPILE)gcc

# Main Makefile target 'all' - it iterates over all targets listed in $(TARGET)
# variable.
all: $(TARGET)

# Target with compilation rules to compile object from source files
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

# Makefile target with rules how to link executable for each target from $(TARGET) list.
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

# Clean target - when called it cleans all object files and executables.
clean:
	rm -f $(TARGET) *.o $(OBJS)
