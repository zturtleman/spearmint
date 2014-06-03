#!/bin/sh

failed=0;
game_failed=0;
bspc_failed=0;

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
	echo "Build successful.";
fi

exit $failed;

