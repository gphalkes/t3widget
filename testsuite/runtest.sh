#!/bin/bash

DIR="`dirname \"$0\"`"
. "$DIR"/_common.sh


if [ $# -ne 1 ] ; then
	fail "Usage: runtest.sh <dir with test>"
fi

setup_TEST "$1"
cd_workdir

rm *

build_test

../../../../record/src/tdreplay -lreplay.log $REPLAYOPTS $TEST/recording || fail "!! Terminal output is different"

[ "$QUIET" = 1 ] || echo "Test passed" >&2
exit 0
