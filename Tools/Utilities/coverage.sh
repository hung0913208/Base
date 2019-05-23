#!/bin/bash

set -e
unameOut="$(uname -s)"
case "${unameOut}" in
	Linux*)     machine=Linux;;
	Darwin*)    machine=Mac;;
	CYGWIN*)    machine=Cygwin;;
	MINGW*)     machine=MinGw;;
	*)          machine="UNKNOWN:${unameOut}"
esac

if [[ -d $1/Coverage ]]; then
	ROOT=$(pwd)

	cd $1/Coverage || exit -1

	lcov --directory . --capture --output-file coverage.info >& /dev/null
	if [ $? != 0 ]; then
		exit 1
	fi

	lcov --remove coverage.info '/usr/*' --output-file coverage.info >& /dev/null
	if [ $? != 0 ]; then
		exit 2
	fi

	lcov --list coverage.info >& /dev/null
	if [ $? != 0 ]; then
		exit 3
	fi

	if [[ ${#CODECOV_TOKEN} -gt 0 ]]; then
		exec 2>&-
		OUTPUT="$(bash <(curl -s https://codecov.io/bash) 2>&1 | tee /dev/fd/2)"

		exec 2>&1
		if [ $? != 0 ]; then
			echo "[  ERROR  ] Codecov did not collect coverage reports"
			exit 4
		else
			REPORT=$(printf $OUTPUT | grep "\-> View reports at *" | cut -c 24-)

			if [[ ${#REPORT} -gt 0 ]]; then
				echo "[   INFO  ] $REPORT"
			fi
		fi
	else
		if [[ $# -gt 1 ]]; then
			OUTPUT=$2
		else
			OUTPUT=$(pwd)/public_html
		fi

		genhtml -o $OUTPUT coverage.info >& /dev/null

		if [[ ${#REVIEW} -gt 0 ]]; then
			PROTOCOL=$(python -c "print(\"$REVIEW\".split(':')[0])")
			RPATH=$(python -c "print(\"$REVIEW\".split('/')[-1])")
			HOST=$(python -c "print(\"$REVIEW\".split('@')[1].split('/')[0])")
			WEB=$(python -c "print(\".\".join(\"$HOST\".split('.')[1:]))")
			USER=$(python -c "print(\"$REVIEW\".split('/')[2].split(':')[0])")
			PASSWORD=$(python -c "print(\"$REVIEW\".split('/')[2].split(':')[1].split('@')[0])")

			if [[ ${#RPATH} -eq 0 ]] || [[ ${#USER} -eq 0 ]] || [[ ${#PASSWORD} -eq 0 ]]; then
				exit -1
			fi
		fi

		if [[ "$PROTOCOL" = "ftp" ]] && [ $(which ncftpput) ]; then
			# @NOTE: Delete remote old code coverage
			ftp -i -n <<EOF
open $HOST
user $USER $PASSWORD
rmdir $FTP
EOF

			# @NOTE: update the new code coverage
			ncftpput -R -v -u "$USER" -p "$PASSWORD" "$HOST" "$RPATH" "$OUTPUT"
			if [ $? != 0 ]; then
				exit $?
			fi
		elif [[ "$PROTOCOL" = "scp" ]] && [ $(which scp) ] && [ $(which expect) ]; then
			expect -c """
set timeout 1
spawn scp -r $OUTPUT $USER@$HOST/$RPATH
expect yes/no { send yes\r ; exp_continue }
expect password: { send $PASSWORD\r }
expect 100%
sleep 1
"""

			if [ $? != 0 ]; then
				exit $?
			fi
		else
			exit 0
		fi

		echo "[   INFO  ] Review https://$USER.$WEB/$FTP/index.html"
	fi

	cd $ROOT || exit -1
elif [[ "$CC" =~ "clang" ]] || [[ "$CXX" =~ "clang++" ]] || [[ "$machine" == "Mac" ]]; then
	echo "[ WARNING ] The coverage don't support clang or MacOS"
	exit 0
fi
