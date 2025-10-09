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

GIT_COMMIT_HASH := $(shell git rev-parse HEAD)
GIT_TAG := $(shell git describe --tags --always --dirty)

version.h:
	echo "#ifndef VERSION_H" > $@
	echo "#define VERSION_H" >> $@
	echo "const char* GIT_COMMIT = \"$(GIT_COMMIT_HASH)\";" >> $@
	echo "const char* GIT_VERSION_TAG = \"$(GIT_TAG)\";" >> $@
	echo "#endif" >> $@

SHAREDOBJ=\
  config.o\
  keywords.o\
  message.o\
  process-message.o\
  quoted-printable.o\

ALLOBJ=\
  $(SHAREDOBJ)\
  version.h

%.o: %.c $(wildcard *.h) version.h
	$(CC) $(CFLAGS) -c $<

$(BIN): % : %.o $(ALLOBJ) $(UTILSLIB)
	$(CC) $(CFLAGS) -o $@ $< $(ALLOBJ) $(LIBS)

install: $(REALBIN)
	sudo install -o root -g root -m 755 $(REALBIN) /usr/local/bin

clean: NULL
	rm -f $(SHAREDOBJ) $(BIN)
	rm -f *.o

NULL:
