#!/bin/bash

DIR="`dirname \"$0\"`"
. "$DIR"/_common.sh


if [ $# -eq 0 ] ; then
	fail "Usage: viewtest.sh <dir with test> [<subtest>]"
fi

setup_TEST "$1"
[ $# -gt 1 ] && [ -n "$1" ] && SUBTEST=".$2"

tdview $REPLAYOPTS $TEST/recording$SUBTEST || fail "!! Terminal output is different"
