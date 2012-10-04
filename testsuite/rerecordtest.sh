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

[ $# -gt 1 ] && [ -n "$1" ] && SUBTEST=".$2"

../../../../record/src/tdrerecord -s -o $TEST/recording$SUBTEST.new $TEST/recording$SUBTEST || fail "!! Could not rerecord test"
[ -s test.log ] && mv test.log $TEST/test.log$SUBTEST.new

exit 0
