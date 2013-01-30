#!/bin/bash

DIR="`dirname \"$0\"`"
. "$DIR"/_common.sh


if [ $# -eq 0 ] ; then
	fail "Usage: recordtest.sh <test> [<subtest>]"
fi

setup_TEST "$1"
cd_workdir

rm *

build_test

[ $# -gt 1 ] && [ -n "$2" ] && SUBTEST=".$2"

tdrecord -o $TEST/recording$SUBTEST -e T3WINDOW_OPTS $RECORDOPTS ./test -t || fail "!! Could not record test"
[ -s test.log ] && mv test.log $TEST/test.log$SUBTEST

exit 0
