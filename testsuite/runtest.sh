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

unset PRINT_SUBTEST
[ "`ls $TEST/recording* | wc -l`" -gt 1 ] && PRINT_SUBTEST=1

for RECORDING in $TEST/recording* ; do
	SUBTEST="${RECORDING##*recording}"
	[ -n "$PRINT_SUBTEST" ] && echo "-- subtest ${SUBTEST#.}" >&2

	tdreplay -lreplay.log ${REPLAYOPTS:--k1} $TEST/recording${SUBTEST} || fail "!! Terminal output is different"
	if [ -e "$TEST/test.log${SUBTEST}" ] ; then
		if ! diff -q "$TEST/test.log${SUBTEST}" test.log ; then
			fail "!! test.log is different"
		fi
	else
		if [ -s "test.log" ] ; then
			fail "!! test.log is different"
		fi
	fi
done

[ "$QUIET" = 1 ] || echo "Test passed" >&2
exit 0
