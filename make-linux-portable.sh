#!/bin/sh

if [ -z "$1" ]; then
	echo "Usage: $0 <arch> [make options]"
	echo "  arch can be x86 or x86_64"
	exit 0
fi

ARCH=$1
SDL_PREFIX=/home/zack/Local/SDL-2.0.4/build-$ARCH

if [ ! -d $SDL_PREFIX ] ; then
	echo "Change SDL_PREFIX in $0 to point to SDL build output"
	exit 0
fi

SDL_CFLAGS="`$SDL_PREFIX/bin/sdl2-config --cflags`"
SDL_LIBS="`$SDL_PREFIX/bin/sdl2-config --libs`"

# Don't pass arch ($1) to make in $*
shift 1

make ARCH=$ARCH USE_PORTABLE_RPATH=1 SDL_CFLAGS="$SDL_CFLAGS" SDL_LIBS="$SDL_LIBS" $*

# Copy SDL lib to where spearmint will read it from
#mkdir -p build/release-linux-$ARCH/lib/$ARCH/
#cp $SDL_PREFIX/lib/libSDL2-2.0.so.0 build/release-linux-$ARCH/lib/$ARCH/
