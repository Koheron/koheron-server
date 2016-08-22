# Makefile for koheron-server CLI
#
# (c) Koheron

API_INC_PATH = ../C

DEFINES=
CROSS_COMPILE=
ARCH_FLAGS=

# Objects
OBJS_API = $(API_INC_PATH)/kclient.o  $(API_INC_PATH)/sessions.o
OBJS = $(OBJS_API) config_file.o kserver_cli.o	

SRCS = $(subst .o,.c, $(OBJS), ))

# Executable name
TARGET=kserver

CFLAGS= -Wall -Werror -I$(API_INC_PATH) $(DEFINES) -Os # -g
CFLAGS += $(ARCH_FLAGS)

LIBS = -lm # -lpthread -lssl -lcrypto

CC=$(CROSS_COMPILE)gcc

all: $(TARGET)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -f $(TARGET) *.o $(OBJS)
