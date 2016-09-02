# (c) Koheron

CPUS = $(shell nproc 2> /dev/null || echo 1)
MAKEFLAGS += --jobs=$(CPUS)

CROSS_COMPILE=
ARCH_FLAGS=
DEBUG_FLAGS=
OPTIM_FLAGS=
DEFINES=
MIDWARE_PATH=
SERVER=kserverd

# Middleware include path
MIDWARE_INC_PATH=../$(MIDWARE_PATH)

# --------------------------------------------------------------
# Objects to build
# --------------------------------------------------------------

# Objects in KServer
OBJS_KSERVER = core/kserver.o              \
               core/listening_channel.o    \
               core/kserver_commands.o     \
               core/kserver_session.o      \
               core/session_manager.o      \
               core/devices_manager.o      \
               core/peer_info.o            \
               core/websocket.o            \
               core/gason.o                \
               core/config.o               \
               core/crypto/base64.o        \
               core/crypto/sha1.o          \
               core/kserver_syslog.o       \
               core/signal_handler.o

# Objects in Middleware
SRC_MIDWARE = $(shell find $(MIDWARE_INC_PATH) -name '*.cpp')
OBJS_MIDWARE = $(subst .cpp,.o, $(SRC_MIDWARE))

# All objects
OBJS = $(OBJS_MIDWARE) $(OBJS_KSERVER) $(OBJS_KS_DEV) main.o	
# List of raw source files (all object files, renamed from .o to .c)
SRCS = $(subst .o,.c, $(OBJS), ))

# --------------------------------------------------------------
# Toolchains
# --------------------------------------------------------------

# Use Link Time Optimization
CC=$(CROSS_COMPILE)gcc -flto
CCXX=$(CROSS_COMPILE)g++ -flto

# --------------------------------------------------------------
# GCC compiling & linking flags
# --------------------------------------------------------------

INC=-I. -I$(MIDWARE_INC_PATH)

CFLAGS=-Wall -Werror -Wno-unknown-pragmas $(INC) $(DEFINES)

CFLAGS += $(ARCH_FLAGS)
CFLAGS += $(DEBUG_FLAGS)
CFLAGS += $(OPTIM_FLAGS)

CXXFLAGS=$(CFLAGS) -std=c++14 -pthread

# --------------------------------------------------------------
# Libraries
# --------------------------------------------------------------

LIBS = -lm # -lpthread -lssl -lcrypto

# --------------------------------------------------------------
# Targets
# --------------------------------------------------------------

all: $(SERVER)

# http://bruno.defraine.net/techtips/makefile-auto-dependencies-with-gcc/
DEP=$(OBJS:.o=.d)
-include $(DEP)

%.o: %.c
	$(CC) -c $(CFLAGS) -MMD -o $@ $<

%.o: %.cpp
	$(CCXX) -c $(CXXFLAGS) -MMD -o $@ $<

$(SERVER): $(OBJS)
	$(CCXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(SERVER) *.o $(OBJS)
