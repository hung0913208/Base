#!/bin/bash

set -e

exec 5>&1
output=$(valgrind -v --tool=helgrind --free-is-write=yes --error-exitcode=-1 $@ 2>&1 | tee /dev/fd/2 )
exit_code=$?
if [[ ! $(grep "ERROR SUMMARY: 0 errors" <<< "$output") ]] || [[ $exit_code != 0 ]]; then
	if [[ $exit_code != 0 ]]; then
		echo "It seems your system, your libraries or you build flags causes warnings"
		exit_code=4
	else
		echo "Good news!!! Still some minor error here after runing valgrind race condition"
		exit_code=1
	fi
	if [[ ! $(grep "exit: 0 bytes in 0 blocks" <<< "$output") ]]; then
		echo "Please check your code again, still have race condition"
		exit_code=2
	fi
fi
if [ $exit_code != 0 ]; then
	exit $exit_code
else
	valgrind -v --tool=exp-dhat --error-exitcode=-1 $@
	valgrind --tool=cachegrind $@
	echo "Congratulation, you has pass valgrind test"
fi
exec 5>&- # release the extra file descriptors
exit 0
