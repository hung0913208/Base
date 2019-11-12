#!/bin/bash

SCRIPT="$(basename "$0")"
ROOT="$(dirname "$0")"

# @NOTE: print log info
info(){
	if [ $# -eq 2 ]; then
		echo "[   INFO  ]: $1 line ${SCRIPT}:$2" >&2
	else
		echo "[   INFO  ]: $1" >&2
	fi
}

# @NOTE: print log warning
warning(){
	if [ $# -eq 2 ]; then
		echo "[ WARNING ]: $1 line ${SCRIPT}:$2" >&2
	else
		echo "[ WARNING ]: $1" >&2
	fi
}

# @NOTE: print log error and exit immediatedly
error(){
	if [ $# -eq 2 ]; then
		echo "[  ERROR  ]: $1 line ${SCRIPT}:$2"
	else
		echo "[  ERROR  ]: $1 in ${SCRIPT}"
	fi
	exit -1
}

exec_with_timeout(){
	CODE=0

	if [ -d "$2" ]; then
		return 0
	fi

	if [ $1 -le  0 ]; then
		TIMEOUT=60
	else
		TIMEOUT=$1
	fi

	("$2" || touch "/tmp/$(basename "$2").fail") & PID=$!
	(for _ in 0..$1; do
		if [ ! "$(kill -0 $PID >& /dev/null)" ]; then
			exit 0;
		else
			sleep 1;
		fi;
	done;
	kill -9 $PID >& /dev/null) &
	KILLER_PID=$!

	# @NOTE: we don't wait KILLER if the test pass completedly
	wait $PID >& /dev/null
	kill $KILLER_PID >& /dev/null
	wait $KILLER_PID >& /dev/null

	# @NOTE: check if the fail flag existed
	if [ -f "/tmp/$(basename "$2").fail" ]; then
		rm -fr "/tmp/$(basename "$2").fail"
		CODE=1
	fi

	PATTERN=$(python -c """
import os

pattern = '$($SU cat /proc/sys/kernel/core_pattern)'
result = ''
check = False
same = False

for c in pattern:
	if c == '%':
		check = True
	elif check is True:
		check = False

		if c == 'e':
			result += '$(basename "$FILE")'
			same = False
		elif c == 'E':
			result += '!'.join(os.path.realpath('$FILE').split('/'))
			same = False
		elif same is False:
			result += '*'
			same = True
	else:
		result += c
		same = False
else:
	print(result)
""")

	COREFILE=$(find . -maxdepth 1 -name "$PATTERN" | head -n 1)
	if [[ -f "$COREFILE" ]]; then
		CODE=2
	fi

	return $CODE
}

compress_coredump(){
	COREDUMP=$1
	BINARY=$2
	OUTPUT=$3

	if [ -f $OUTPUT ]; then
		rm -fr $OUTPUT
	fi

	info "compressing {$COREDUMP, $BINARY} to $OUTPUT"
	zip $OUTPUT $COREDUMP $BINARY
	return 0
}

coredump(){
	FILE=$1

	if echo "$($SU cat /proc/sys/kernel/core_pattern)" | grep "false"; then
		return 0
	else
		exec 2>&-

		PATTERN=$(python -c """
import os

pattern = '$($SU cat /proc/sys/kernel/core_pattern)'
result = ''
check = False
same = False

for c in pattern:
	if c == '%':
		check = True
	elif check is True:
		check = False

		if c == 'e':
			result += '$(basename "$FILE")'
			same = False
		elif c == 'E':
			result += '!'.join(os.path.realpath('$FILE').split('/'))
			same = False
		elif same is False:
			result += '*'
			same = True
	else:
		result += c
		same = False
else:
	print(result)
""")

		find . -maxdepth 1 -name "$PATTERN"
		COREFILE=$(find . -maxdepth 1 -name "$PATTERN" | head -n 1)
		exec 2>&1

		if [[ -f "$COREFILE" ]]; then
			info "check coredump of $FILE -> found $COREFILE"
			echo ""
		else
			info "check coredump of $FILE -> not found"
		fi

		if [[ -f "$COREFILE" ]]; then
			gdb -c "$COREFILE" "$FILE" -ex "bt" -ex "set pagination 0" -batch;
			compress_coredump $COREFILE $FILE "$(pwd)/$(git rev-parse --verify HEAD).zip"

			if [ -f "$(pwd)/$(git rev-parse --verify HEAD).zip" ]; then
				echo ""
				info "send  $(pwd)/$(git rev-parse --verify HEAD).zip to $EMAIL_AUTHOR"

				if [ -f $ROOT/Tools/Utilities/fsend.sh ]; then
					$ROOT/Tools/Utilities/fsend.sh upload "$(pwd)/$(git rev-parse --verify HEAD).zip" $EMAIL_AUTHOR
					echo ""
				elif [ -f $BASE/Tools/Utilities/fsend.sh ]; then
					$BASE/Tools/Utilities/fsend.sh upload "$(pwd)/$(git rev-parse --verify HEAD).zip" $EMAIL_AUTHOR
					echo ""
				else
					warning "can't find tool fsend.sh to upload $(pwd)/$(git rev-parse --verify HEAD).zip to $EMAIL_AUTHOR"
				fi
			fi
		fi
	fi
}

if [ "$1" = "Coverage" ]; then
	unameOut="$(uname -s)"
	case "${unameOut}" in
		Linux*)     machine=Linux;;
		Darwin*)    machine=Mac;;
		CYGWIN*)    machine=Cygwin;;
		MINGW*)     machine=MinGw;;
		FreeBSD*)   machine=FreeBSD;;
		*)          machine="UNKNOWN:${unameOut}"
	esac
fi

LLVM_SYMBOLIZER=$(find /usr/bin/ -name llvm-symbolizer)
ROOT=$(git rev-parse --show-toplevel)
CURRENT=$(pwd)

if [[ -d "${ROOT}/Base" ]]; then
	BASE="${ROOT}/Base"
elif [[ -d "${ROOT}/base" ]]; then
	BASE="${ROOT}/base"
elif [[ -d "${ROOT}/LibBase" ]]; then
	BASE="${ROOT}/LibBase"
elif [[ -d "${ROOT}/Eevee" ]]; then
	BASE="${ROOT}/Eevee"
fi

if [[ $# -ge 2 ]]; then
	WAIT=$1
else
	WAIT=60
fi

if [ -d "./$1" ]; then
	cd "$1" || error "can't cd to $1"
	CODE=0

	if [ $(whoami) != "root" ]; then
		if [ ! $(which sudo) ]; then
			error "Sudo is needed but you aren"t on root and can"t access to root"
		fi

		if sudo -S -p "" echo -n < /dev/null 2> /dev/null ; then
			info "I found sudo can run on account $(whoami)"
			SU="sudo"
		else
			warning "Sudo is not enabled"
		fi
	fi

	# @NOTE: enable coredump produce since some system may hidden it
	$SU sysctl -w kernel.core_pattern="core-%e.%p.%h.%t"
	ulimit -c unlimited

	if [ "$1" == "Debug" ] || [ "$1" == "Release" ] || [ "$1" == "Coverage" ] || [ "$1" == "Sanitize" ] || [ "$1" == "Profiling" ]; then
		export ASAN_SYMBOLIZER_PATH=$LLVM_SYMBOLIZER
		export LSAN_OPTIONS=verbosity=1:log_threads=1
		export TSAN_OPTIONS=second_deadlock_stack=1

		echo "Run directly on this environment"
		echo "=============================================================================="

		FAIL=0
		if [ "$1" != "Coverage" ] && [ "$1" != "Sanitize" ] && [ "$1" != "Profiling" ]; then
			if ! ctest --verbose --timeout 1; then
				FAIL=1
			fi
		else
			IDX=0
			FAILLIST=()

			for FILE in ./Tests/*; do
				if [ -x "$FILE" ] && [ ! -d "$FILE" ]; then
					IDX=$((IDX+1))

					echo "        Start  $IDX: $(basename $FILE)"
					echo "Test command: $FILE"
					echo ""

					if ! exec_with_timeout -1 $FILE; then
						FAILLIST=("${FAILLIST[@]}" "$(basename $FILE)")

						echo "$IDX/ Test  #$IDX:  .............................   Failed"
						echo ""
						FAIL=1
					else
						echo "$IDX/ Test  #$IDX:  .............................   Passed"
						echo ""
					fi
				fi
			done

			if [[ $FAIL -ne 0 ]]; then
				echo "------------------------------------------------------------------------------"
				info "you suite has failed at:"
	
				for NAME in ${FAILLIST[@]}; do
					echo "  - $NAME"
				done
				echo ""
			fi
		fi

		echo "------------------------------------------------------------------------------"
		echo ""

		if [[ $FAIL -ne 0 ]]; then
			EMAIL_AUTHOR=$(git --no-pager show -s --format='%ae' HEAD)

			if ! echo "$($SU cat /proc/sys/kernel/core_pattern)" | grep "false" >& /dev/null; then
				echo "Check coredump"
				echo "=============================================================================="

				for FILE in ./Tests/*; do
					if [ -x "$FILE" ]; then
						coredump $FILE
					fi
				done
				echo "------------------------------------------------------------------------------"
				echo ""
			fi
		fi

		if [ "$1" = "Sanitize" ]; then
			if [[ $# -gt 1 ]]; then
				if [ $2 == "symbolize" ]; then
					export ASAN_OPTIONS=symbolize=1
					echo "Run with ASAN_SYMBOLIZER"
					echo "=============================================================================="

					for FILE in ./Tests/*; do
						if [ -x "$FILE" ] && [ ! -d "$FILE" ]; then
							if ! $FILE; then
								coredump $FILE
							fi
						fi
					done
					echo "------------------------------------------------------------------------------"
					echo ""
				elif [ $2 == "leak" ]; then
					export ASAN_OPTIONS=symbolize=1
					echo "Run with ASAN_LEAK"
					echo "=============================================================================="

					for FILE in ./Tests/*; do
						if [ -x "$FILE" ] && [ ! -d "$FILE" ]; then
							if ! $FILE; then
								coredump $FILE
							fi
						fi
					done
					echo "------------------------------------------------------------------------------"
					echo ""
				fi
			fi
		fi

		if [[ $FAIL -eq 0 ]] && [[ $# -ge 2 ]] && [ "$1" != "Sanitize" ]; then
			# @NOTE: test with gdb in order to verify slowing down effect to detect rare issues

			echo "Run with GDB"
			echo "=============================================================================="

			for FILE in ./Tests/*; do
				if [ -x "$FILE" ] && [ ! -d "$FILE" ]; then
					exec 2>&-
					OUTPUT=$(gdb -ex="set confirm on" -ex=run -ex="bt" -ex=quit --args $FILE 1>& /dev/null | tee /dev/fd/2)
					exec 2>&1

					echo "$OUTPUT"

					if ! echo "$OUTPUT" | grep "exited normally" >& /dev/null; then
						FAIL=-2

						if ! echo "$OUTPUT" | grep "exited with code" >& /dev/null; then
							FAIL=-1
						fi
					fi
				fi
			done

			echo "------------------------------------------------------------------------------"
			echo ""
		fi
	else
		if ! "$(ctest --verbose)"; then
			EMAIL_AUTHOR=$(git --no-pager show -s --format='%ae' HEAD)

			if ! echo "$($SU cat /proc/sys/kernel/core_pattern)" | grep "false" >& /dev/null; then
				echo "Check coredump"
				echo "=============================================================================="
	
				for FILE in ./Tests/*; do
					if [ -x "$FILE" ]; then
						coredump $FILE
					fi
				done
				echo "------------------------------------------------------------------------------"
				echo ""
			fi
			exit -1
		fi
	fi
	cd "$CURRENT" || error "can't cd to $CURRENT"
	exit $FAIL
else
	error "Please run \"reinstall.sh $1\" first"
fi
