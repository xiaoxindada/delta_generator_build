#!/bin/bash
LOCALDIR=$(pwd)

function apply() {
	cd $1
	for p in $(find . -type f -name "*.patch"); do
		patch --directory $2 --batch -f -N -s -p1 < $p
	done
	cd $LOCALDIR
}

apply $LOCALDIR/patches/logging $LOCALDIR/src/logging
apply $LOCALDIR/patches/libchrome $LOCALDIR/src/libchrome
echo "apply patch done!"
