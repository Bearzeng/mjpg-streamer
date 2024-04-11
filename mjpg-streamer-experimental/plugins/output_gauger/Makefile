###############################################################
#
# Purpose: Makefile for CppSQLite3
# Author.: lqi
#
###############################################################

CC = $(CROSS_COMPILE)g++
AR := ar

CFLAGS += -L. -DLINUX -D_GNU_SOURCE -Wall -fPIC
CFLAGS += -g

TARGET := libcppsqlite3.a
OUTDIR := ./output

SRCS := $(wildcard *.c)
SRCS += $(wildcard *.cpp)
SRCS := $(notdir $(SRCS))
OBJS := $(SRCS:%.cpp=$(OUTDIR)/%.o)
OBJS := $(OBJS:%.c=$(OUTDIR)/%.o)
DEPS := $(OBJS:%.o=%.d)

all: $(OUTDIR) $(TARGET)
        
$(OUTDIR):
	@if [ ! -r $(OUTDIR) ]; then mkdir -p $(OUTDIR); fi

$(TARGET): $(OBJS)
	$(AR) -r $(OUTDIR)/$@ $^

$(OUTDIR)/%.o: %.cpp
	$(CC) $(CFLAGS) $(DEFINE) -o $@ -c $<

$(OUTDIR)/%.d: %.c
	$(CC) -M $(CFLAGS) $< > $@
	$(CC) -M $(CFLAGS) $< | sed s/\ \.o/.d/ > $@

$(OUTDIR)/%.d: %.cpp
	$(CC) -M $(CFLAGS) $< > $@
	$(CC) -M $(CFLAGS) $< | sed s/\ \.o/.d/ > $@

clean:
	rm -f $(OUTDIR)/* $(TARGET)

