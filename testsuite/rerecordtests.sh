#!/bin/bash

cd "$(dirname "$0")"

confirm() {
	unset CONFIRM
	while [[ -z $CONFIRM ]] ; do
		read CONFIRM
	done
}

for TEST in tests/* ; do
	for RECORDING in $TEST/recording* ; do
		if [[ ${RECORDING%recording} != $RECORDING ]] ; then
			SUBTEST=
			./rerecordtest.sh $TEST || { read ; continue ; }
		else
			SUBTEST=${RECORDING##*.}
			./rerecordtest.sh $TEST $SUBTEST || { read ; continue ; }
		fi
		dwdiff -Pc -C0 -L $RECORDING{,.new}

		echo "Do you want to save these changes? "
		confirm
		if [[ $CONFIRM = y ]] ; then
			mv $RECORDING{.new,}
			[ -s $TEST/test.log${SUBTEST:+.$SUBTEST}.new ] && mv $TEST/test.log${SUBTEST:+.$SUBTEST}{.new,}
		else
			echo "Do you want to remove the new files? "
			confirm
			if [[ $CONFIRM = y ]] ; then
				rm $RECORDING.new 2>/dev/null
				rm $TEST/test.log${SUBTEST:+.$SUBTEST}.new 2>/dev/null
			fi
		fi
	done
done
