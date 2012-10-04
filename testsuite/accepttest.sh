#!/bin/bash

DIR="`dirname \"$0\"`"
. "$DIR"/_common.sh


if [ $# -eq 0 ] ; then
	fail "Usage: showtestdiff.sh <test> [<subtest>]"
fi

setup_TEST "$1"
[ $# -gt 1 ] && [ -n "$2" ] && SUBTEST=".$2"

mv $TEST/recording$SUBTEST.new $TEST/recording$SUBTEST
if [ -e $TEST/test.log$SUBTEST.new ] ; then
	mv $TEST/test.log$SUBTEST.new $TEST/test.log$SUBTEST
else
	rm $TEST/test.log$SUBTEST 2>/dev/null
fi
exit 0
