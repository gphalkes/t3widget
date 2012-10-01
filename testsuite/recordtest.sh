#!/bin/bash

DIR="`dirname \"$0\"`"
. "$DIR"/_common.sh


if [ $# -eq 0 ] ; then
	fail "Usage: recordtest.sh <test>"
fi

TEST="$PWD/$1"

shift
cd_workdir

rm *

build_test

../../../../record/src/tdrecord -o $TEST/recording -e T3WINDOW_OPTS $RECORDOPTS ./test -t || fail "!! Could not record test"

exit 0
