CC = gcc
CFLAGS = -Wall -c `pkg-config --cflags libsbl aravis-0.10 libpng`

LDFLAGS = -lm `pkg-config --libs libsbl aravis-0.10 libpng`

PREFIX ?= /usr/local

BINARIES = build/sblv_print \
		   build/sblv_extract_one_frame

all: build $(BINARIES)

build/sbl_get_version: build/sbl_get_version.o
	$(CC) $(LDFLAGS) -o $@ $<

build/sbl_find_video: build/sbl_find_video.o
	$(CC) $(LDFLAGS) -o $@ $<

build/sbl_cam_list: build/sbl_cam_list.o
	$(CC) $(LDFLAGS) -o $@ $<

build/sbl_cam_grab_frame: build/sbl_cam_grab_frame.o
	$(CC) $(LDFLAGS) -o $@ $<

build:
	mkdir -p build

build/%.o: src/%.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f build/*.o

distclean:
	rm -fr build

install:
	install -D -m 755 build/sbl_get_version  $(DESTDIR)$(PREFIX)/bin/sbl_get_version
	install -D -m 755 build/sbl_find_video   $(DESTDIR)$(PREFIX)/bin/sbl_find_video
	install -D -m 755 build/sbl_cam_list     $(DESTDIR)$(PREFIX)/bin/sbl_cam_list
	install -D -m 755 build/sbl_cam_grab_frame   $(DESTDIR)$(PREFIX)/bin/sbl_cam_grab_frame

.PHONY: all clean distclean install

