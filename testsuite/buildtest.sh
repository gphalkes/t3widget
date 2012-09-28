#!/bin/bash

DIR="`dirname \"$0\"`"
. "$DIR"/_common.sh


if [ $# -ne 1 ] ; then
	fail "Usage: buildtest.sh <dir with test>"
fi

setup_TEST "$1"
cd_workdir

rm *

build_test
