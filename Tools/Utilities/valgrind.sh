#!/bin/bash

exec 5>&1
output=$( valgrind -v --tool=memcheck --leak-check=full --leak-resolution=high --track-origins=yes --show-leak-kinds=all --error-exitcode=-1 --trace-children=yes $@ 2>&1 | tee /dev/fd/2 )
exit_code_1=$?

if [[ ! $(grep "ERROR SUMMARY: 0 errors" <<< "$output") ]] || [[ $exit_code_1 != 0 ]]; then
	if [[ $exit_code_1 != 0 ]]; then
		echo "It seems your system, your libraries or you build flags causes warnings"
		exit_code_1=4
	else
		echo "Good news!!! Still some minor error here after runing valgrind test"
		exit_code_1=1
	fi
	if [[ ! $(grep "exit: 0 bytes in 0 blocks" <<< "$output") ]]; then
		echo "Please check your code again, still have memory leak"
		exit_code_1=2
	fi
elif [[ ! $(grep "in use at exit: 0 bytes in 0 blocks" <<< "$output") ]]; then
	echo "It seems you have memory leak with the child processes"
fi

if [ $exit_code_1 != 0 ]; then
	exit $exit_code_1
else
	valgrind -v --tool=exp-dhat --error-exitcode=-1 $@
	valgrind --tool=cachegrind $@
	echo "Congratulation, you has pass valgrind test"
fi
exec 5>&- # release the extra file descriptors
exit 0
