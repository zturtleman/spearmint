#!/bin/sh

failed=0;

# check if testing mingw
if [ "$CC" = "i686-w64-mingw32-gcc" ]; then
	MAKE=./cross-make-mingw.sh
else
	MAKE=make
fi

# Default Build
($MAKE clean release) || failed=1;

if [ $failed -eq 1 ]; then
	echo "Build failure.";
else
	echo "All builds successful.";
fi

exit $failed;

