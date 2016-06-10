
API_INC_PATH = ..

CFLAGS= -Wall -Werror -I$(API_INC_PATH) -march=native -std=c99 -O3 # -g
TARGET = hello_world

# Objects
OBJS_API = $(API_INC_PATH)/kclient.o  \
           $(API_INC_PATH)/sessions.o

OBJS = $(OBJS_API) hello_world.o
SRCS = $(subst .o,.c, $(OBJS), ))

CC=gcc 

all: $(TARGET)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS) $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o $(OBJS)