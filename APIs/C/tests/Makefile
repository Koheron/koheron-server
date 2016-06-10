# Makefile for C API tests
#
# (c) Koheron

#TARGET_HOST = arm
#TARGET_HOST = local
#TARGET_HOST = Win32
#TARGET_HOST = Win64

API_INC_PATH = ..

# GCC compiling & linking flags
CFLAGS= -Wall -Werror -I$(API_INC_PATH) $(DEFINES) -std=c99 -O2 # -g

# Toolchain
ifeq ($(TARGET_HOST),armhf)
CROSS_COMPILE=arm-linux-gnueabihf-
TARGET=tests
ARM_FLAGS = -march=armv7-a -mtune=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard
CFLAGS += $(ARM_FLAGS)
else ifeq ($(TARGET_HOST),arm)
ARM_FLAGS = -march=armv7-a -mtune=cortex-a9
CFLAGS += $(ARM_FLAGS)
CROSS_COMPILE=arm-linux-gnueabi-
# CROSS_COMPILE=arm-xilinx-linux-gnueabi-
TARGET=tests
else ifeq ($(TARGET_HOST),local)
CROSS_COMPILE?=
DEFINES += -DLOCAL
CFLAGS += -march=native
TARGET=tests
else ifeq ($(TARGET_HOST),Win32)
CROSS_COMPILE=i686-w64-mingw32-
TARGET=tests.exe
LDFLAGS += -lws2_32
else ifeq ($(TARGET_HOST),Win64)
CROSS_COMPILE=x86_64-w64-mingw32-
TARGET=tests.exe
LDFLAGS += -lws2_32
endif

# Libraries
LIBS = -lm # -lpthread -lssl -lcrypto

# Objects
OBJS_API = $(API_INC_PATH)/kclient.o  \
           $(API_INC_PATH)/sessions.o

OBJS = $(OBJS_API) main.o
# List of raw source files (all object files, renamed from .o to .c)
SRCS = $(subst .o,.c, $(OBJS), ))

# Main GCC executable (used for compiling and linking)
CC=$(CROSS_COMPILE)gcc -flto

# Main Makefile target 'all' - it iterates over all targets listed in $(TARGET)
# variable.
all: $(TARGET)

# Target with compilation rules to compile object from source files
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

# Makefile target with rules how to link executable for each target from $(TARGET) list.
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)

# Clean target - when called it cleans all object files and executables.
clean:
	rm -f $(TARGET) *.o $(OBJS)
