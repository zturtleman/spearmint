#!/bin/sh

if [ -z "$1" ]; then
	echo "Usage: $0 <arch>"
	echo "  arch can be x86 or x86_64"
	exit 0
fi

ARCH=$1
SDL_PREFIX=/home/zack/Local/SDL-2.0.3/build-$ARCH

SDL_CFLAGS="`$SDL_PREFIX/bin/sdl2-config --cflags`"
SDL_LIBS="`$SDL_PREFIX/bin/sdl2-config --libs`"

make ARCH=$ARCH USE_PORTABLE_RPATH=1 BUILD_FINAL=1 SDL_CFLAGS="$SDL_CFLAGS" SDL_LIBS="$SDL_LIBS"

