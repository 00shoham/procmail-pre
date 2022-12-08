REALBIN=\
  procmail-pre

BIN=\
  config-test\
  $(REALBIN)\

all: $(BIN)

LUADIR := $(shell /usr/bin/find /usr/include -type d -name 'lua*' | sort | tail -1)
LUAPARTS := $(subst /, ,$(LUADIR))
LUAVER := $(lastword $(LUAPARTS))

UTILDIR=../../idanlib/src
UTILSLIB=$(UTILDIR)/utils.a

INCLUDES=-I$(LUADIR) -I$(UTILDIR)
LIBS=$(UTILSLIB) -lm -lpthread -luuid -l$(LUAVER) -lcurl

CC=gcc
CFLAGS=-Wall -O2 $(INCLUDES)
# CFLAGS=-Wall -g -rdynamic -fsanitize=address $(INCLUDES)

SHAREDOBJ=\
  config.o\
  keywords.o\
  message.o\
  process-message.o\
  quoted-printable.o\

ALLOBJ=\
  $(SHAREDOBJ)\

%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c $<

$(BIN): % : %.o $(ALLOBJ) $(UTILSLIB)
	$(CC) $(CFLAGS) -o $@ $< $(ALLOBJ) $(LIBS)

install: $(REALBIN)
	sudo install -o root -g root -m 755 $(REALBIN) /usr/local/bin

clean: NULL
	rm -f $(SHAREDOBJ) $(BIN)
	rm -f *.o

NULL:
