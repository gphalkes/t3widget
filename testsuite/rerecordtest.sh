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

tdrerecord -o $TEST/recording$SUBTEST.new $TEST/recording$SUBTEST || fail "!! Could not rerecord test"
[ -s test.log ] && mv test.log $TEST/test.log$SUBTEST.new

if ! tdcompare -v $TEST/recording$SUBTEST{,.new} ; then
	echo -e "\\033[31;1mTest $TEST${SUBTEST:+ $SUBTEST} has different visual result\\033[0m"
	exit 1
fi

exit 0
