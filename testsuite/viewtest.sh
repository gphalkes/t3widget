#!/bin/bash

DIR="`dirname \"$0\"`"
. "$DIR"/_common.sh


if [ $# -ne 1 ] ; then
	fail "Usage: runtest.sh <dir with test>"
fi

setup_TEST "$1"

$DIR/../../../record/src/tdview $REPLAYOPTS $TEST/recording || fail "!! Terminal output is different"
