#!/bin/bash

DIR="`dirname \"$0\"`"
. "$DIR"/_common.sh


if [ $# -eq 0 ] ; then
	fail "Usage: showtestdiff.sh <test> [<subtest>]"
fi

setup_TEST "$1"
[ $# -gt 1 ] && [ -n "$2" ] && SUBTEST=".$2"

dwdiff -Pc -C3 $TEST/recording$SUBTEST $TEST/recording$SUBTEST.new
diff -u $TEST/test.log$SUBTEST $TEST/test.log$SUBTEST.new
exit 0
