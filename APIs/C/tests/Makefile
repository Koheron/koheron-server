# Makefile for C API tests
#
# (c) Koheron

#TARGET_HOST = arm
#TARGET_HOST = local

API_INC_PATH = ..

# Toolchain
ifeq ($(TARGET_HOST),arm)
#CROSS_COMPILE?=arm-linux-gnueabi-
CROSS_COMPILE?=arm-linux-gnueabihf-
#CROSS_COMPILE?=arm-xilinx-linux-gnueabi-
else ifeq ($(TARGET_HOST),local)
CROSS_COMPILE?=
DEFINES += -DLOCAL
endif

# Objects
OBJS_API = $(API_INC_PATH)/kclient.o  \
           $(API_INC_PATH)/sessions.o

OBJS = $(OBJS_API) main.o
# List of raw source files (all object files, renamed from .o to .c)
SRCS = $(subst .o,.c, $(OBJS), ))

# Executable name
TARGET=tests

# GCC compiling & linking flags
CFLAGS= -Wall -Werror -I$(API_INC_PATH) $(DEFINES) -Os # -g

ifeq ($(TARGET_HOST),arm)
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
