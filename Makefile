CC = gcc
CFLAGS = -Wall -c `pkg-config --cflags libsbl aravis-0.10 libpng`

LDFLAGS = -lm `pkg-config --libs libsbl aravis-0.10 libpng` -lavformat -lavcodec -lavutil

PREFIX ?= /usr/local

BINARIES = build/sblv_print \
		   build/sblv_extract_one_frame \
		   build/sblv_export

all: build $(BINARIES)

build:
	mkdir -p build

build/%.o: src/%.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f build/*.o

distclean:
	rm -fr build

install:
	install -D -m 755 build/sblv_print  $(DESTDIR)$(PREFIX)/bin/sblv_print
	install -D -m 755 build/sblv_extract_one_frame   $(DESTDIR)$(PREFIX)/bin/sblv_extract_one_frame

.PHONY: all clean distclean install

