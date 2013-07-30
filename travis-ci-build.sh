#!/bin/sh

failed=0;
game_failed=0;
bspc_failed=0;

# check if testing mingw
if [ "$CC" = "i686-w64-mingw32-gcc" ]; then
	export PLATFORM=mingw32
	export ARCH=x86
	export CC=
fi

# Game Default Build
(make clean release) || game_failed=1;

# BSPC Default Build, bot map AI compiler
(make -C code/bspc clean release) || bspc_failed=1;

if [ $game_failed -eq 1 ]; then
	echo "Game build failure.";
	failed=1;
else
	echo "Game build successful.";
fi

if [ $bspc_failed -eq 1 ]; then
	echo "BSPC build failure.";
	failed=1;
else
	echo "BSPC build successful.";
fi

if [ $failed -eq 1 ]; then
	echo "Build failure.";
else
	echo "All builds successful.";
fi

exit $failed;

