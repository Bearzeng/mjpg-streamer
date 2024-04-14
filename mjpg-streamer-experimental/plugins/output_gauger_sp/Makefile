###############################################################
#
# Purpose: Makefile for "M-JPEG Streamer"
# Author.: Tom Stoeveken (TST)
# Version: 0.3
# License: GPL
#
###############################################################

CC = $(CROSS_COMPILE)g++

OTHER_HEADERS = ../../mjpg_streamer.h ../../utils.h ../output.h ../input.h

CFLAGS += -L. -DLINUX -D_GNU_SOURCE -Wall -shared -fPIC
CFLAGS += -g
#CFLAGS += -DDEBUG
LFLAGS += -lpthread -ldl -lgauger -lsqlite3

ifeq ("$(ARCH)", "ARM")
CFLAGS += -L ./arm
else
CFLAGS += -L ./x64
endif

SRCS := $(wildcard *.c)
SRCS += $(wildcard *.cpp)
SRCS := $(notdir $(SRCS))
OBJS := $(SRCS:%.cpp=%.o)
OBJS := $(OBJS:%.c=%.o)
DEPS := $(OBJS:%.o=%.d)

TARGET := output_gauger.so

all: $(TARGET)

%.o: %.cpp
	$(CC) $(CFLAGS) $(DEFINE) -o $@ -c $<


%.d: %.c
	$(CC) -M $(CFLAGS) $< > $@
	$(CC) -M $(CFLAGS) $< | sed s/\ \.o/.d/ > $@

%.d: %.cpp
	$(CC) -M $(CFLAGS) $< > $@
	$(CC) -M $(CFLAGS) $< | sed s/\ \.o/.d/ > $@

clean:
	rm -f *.a *.o core *~ *.lo *.d $(TARGET)

$(TARGET): $(OTHER_HEADERS) $(DEPS) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LFLAGS)
