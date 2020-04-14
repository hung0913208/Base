#!/bin/bash

unameOut="$(uname -s)"
case "${unameOut}" in
	Linux*)     machine=Linux;;
	Darwin*)    machine=Mac;;
	CYGWIN*)    machine=Cygwin;;
	MINGW*)     machine=MinGw;;
	*)          machine="UNKNOWN:${unameOut}"
esac

function generate_codecov_yaml() {
	cat > "$(git rev-parse --show-toplevel)/codecov.yml" << EOF
codecov:
  token: "$CODECOV_TOKEN"
  bot: "codecov-io"
  ci:
    - "$(hostname)"
  fixes:
    - "::$(basename $1)/Coverage"
EOF
}

if [[ $machine == "Linux" ]]; then
	SONARCLOUD_CLI="https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-3.3.0.1492-linux.zip"
	SONARCLOUD_BUILD="https://sonarcloud.io/static/cpp/build-wrapper-linux-x86.zip"
	BUILD_WARPER="build-wrapper-linux-x86-64"
elif [[ $machine == "Mac" ]]; then
	SONARCLOUD_CLI="https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-3.3.0.1492-linux.zip"
	SONARCLOUD_BUILD="https://sonarcloud.io/static/cpp/build-wrapper-macosx-x86.zip"
	BUILD_WARPER="build-wrapper-macosx-x86"
fi

if [[ -d $1/Coverage ]]; then
	ROOT=$(realpath $(pwd))

	cd $1/Coverage || exit -1

	lcov --directory . --capture --output-file coverage.info &> /dev/null
	if [ $? != 0 ]; then
		exit 1
	fi

	lcov --remove coverage.info '/usr/*' --output-file coverage.info &> /dev/null

	if ! lcov --list coverage.info; then
		exit 3
	fi

	if [[ ${#COVERALLS_REPO_TOKEN} -gt 0 ]]; then
		if pip install --user cpp-coveralls &> /dev/null; then
			echo "[   INFO  ] pushing coverage to coveralls:
---------------------------------------------------------------------------------------------------

$(coveralls --gcov-options '\-lp')
"
		fi
	fi

	if [[ ${#CODECOV_TOKEN} -gt 0 ]]; then
		exec 2>&-
		generate_codecov_yaml $ROOT
		echo "[   INFO  ] pushing coverage to codecov using this config:
---------------------------------------------------------------------------------------------------

$(cat $(git rev-parse --show-toplevel)/codecov.yml)
"
		if ! bash <(curl -s https://codecov.io/bash -v) &> /tmp/codecov.log; then
			echo "[  ERROR  ] Codecov did not collect coverage reports, here is the log:
---------------------------------------------------------------------------------------------------

$(cat /tmp/codecov.log)
"
			exit 4
		else
			exec 2>&1
			rm -fr /tmp/codecov.log
		fi
	fi

	if [[ ${#SONARCLOUD_TOKEN} -gt 0 ]]; then
		PATH="$PATH:$(pwd)/sonar/build:$(pwd)/sonar/cli/bin"
		mkdir -p $(pwd)/sonar/build
		mkdir -p $(pwd)/sonar/cli

		if [[ ${#SONARCLOUD_CLI} -gt 0 ]]; then
			unzip $SONARCLOUD_CLI -d $(pwd)/sonar/cli
		else
			error "can't find sonarcloud tools"
		fi

		if [[ ${#SONARCLOUD_BUILD} -gt 0 ]]; then
			unzip $SONARCLOUD_BUILD -d $(pwd)/sonar/build

			if [ $? = 0 ]; then
				$BUILD_WARPER --out-dir ./bw-output make clean all
			fi
		fi

		KEY=$(python - c "print(\"$SONARCLOUD_TOKEN\".split(':')[0]")
		TOKEN=$(python - c "print(\"$SONARCLOUD_TOKEN\".split(':')[2]")
		ORGAN=$(python - c "print(\"$SONARCLOUD_TOKEN\".split(':')[1]")

		sonar-scanner 						 \
			-Dsonar.projectKey=$KEY				 \
			-Dsonar.organization=$ORGAN 			 \
			-Dsonar.sources=$SOURCE 			 \
			-Dsonar.cfamily.build-wrapper-output=./bw-output \
			-Dsonar.host.url=https://sonarcloud.io 		 \
			-Dsonar.login="$TOKEN"
	fi

	if [[ ${#REVIEW} -gt 0 ]]; then
		if [[ $# -gt 1 ]]; then
			OUTPUT=$2
		else
			OUTPUT=$(pwd)/public_html
		fi

		if ! genhtml -o "$OUTPUT" coverage.info &> /dev/null; then
			exit -1
		fi

		PROTOCOL=$(python -c "print(\"$REVIEW\".split(':')[0])")
		RPATH=$(python -c "print(\"/\".join(\"$REVIEW\".split('/')[3:]))")
		HOST=$(python -c "print(\"$REVIEW\".split('@')[1].split(':')[0])")
		WEB=$(python -c "print(\".\".join(\"$HOST\".split('.')[1:]))")
		USER=$(python -c "print(\"$REVIEW\".split('/')[2].split(':')[0])")
		PASSWORD=$(python -c "print(\"$REVIEW\".split('/')[2].split(':')[1].split('@')[0])")

		if [[ ${#USER} -eq 0 ]] || [[ ${#PASSWORD} -eq 0 ]]; then
			exit -1
		fi

		if python -c "print(\"$REVIEW\".split('/')[2].split(':')[2])" &> /dev/null; then
			PORT=$(python -c "print(\"$REVIEW\".split('/')[2].split(':')[2])")
		else
			PORT=0
		fi

		if [[ "$PROTOCOL" = "ftp" ]] && [ "$(which lftp)" ]; then
			# @NOTE: Delete remote old code coverage
			lftp $HOST -u $USER,$PASSWORD -e "set ftp:ssl-allow no;" <<EOF
rm -f $RPATH/*
EOF

			cd $OUTPUT || exit -1
			# @NOTE: update the new code coverage
		
			if [[ ${#RPATH} -eq 0 ]]; then
				RPATH="/"
			fi

			if ncftpput -d /tmp/coverage.log -R -V -u "$USER" -p "$PASSWORD" "$HOST" "$RPATH" ./; then
				rm -fr /tmp/coverage.log
			else
				cat /tmp/coverage.log && rm -fr /tmp/coverage.log

				if ! lftp  -e "set ftp:ssl-allow no; mirror -R $(pwd) $RPATH" -u $USER,$PASSWORD $HOST; then
					exit -1
				fi
			fi
		elif [[ "$PROTOCOL" = "scp" ]] && [ "$(which scp)" ] && [ "$(which expect)" ]; then
			if [[ ${PORT} -gt 0 ]]; then
				PORT="-P $PORT"
			else
				PORT=''
			fi

			expect -c """
set timeout 600
spawn scp -q -o StrictHostKeyChecking=no $PORT -r $OUTPUT $USER@$HOST/$RPATH
expect password: { send $PASSWORD\r }
expect 100% { interact; }
sleep 1
"""

			if [ $? != 0 ]; then
				exit $?
			fi
		elif [[ "$PROTOCOL" = "sftp" ]] && [ "$(which sftp)" ] && [ "$(which expect)" ]; then
			CURRENT=$(pwd)

			if [[ ${PORT} -gt 0 ]]; then
				PORT="-P $PORT"
			else
				PORT=''
			fi

			cd $OUTPUT

			expect -d -c """
set timeout 600

spawn sftp -q -o StrictHostKeyChecking=no $PORT $USER@$HOST
expect \"Password:\" { send \"$PASSWORD\n\" }
expect \"sftp>\" { send \"cd $RPATH\n\" }
expect \"sftp>\" { send \"put -r .\n\" }
expect \"sftp>\" { send \"exit\n\"; interact }
"""
			CODE=$?

			cd $CURRENT

			if [ $CODE != 0 ]; then	
				echo "can't upload coverage successful"
				exit $CODE
			fi
		else
			exit 0
		fi

		if [[ ${#WEBINDEX} -gt 0 ]]; then
			echo "[   INFO  ] Review $WEBINDEX"
		elif [ $RPATH = '/' ]; then
			echo "[   INFO  ] Review https://${USER}.${WEB}/index.html"
		else
			echo "[   INFO  ] Review https://${USER}.${WEB}/$RPATH/index.html"
		fi
	else
		genhtml -o "./gcov" coverage.info >& /dev/null
		exit $?
	fi

	cd $ROOT || exit -1
elif [[ "$CC" =~ 'clang'* ]] || [[ "$CXX" =~ 'clang++'* ]] || [[ "$machine" == "Mac" ]]; then
	echo "[ WARNING ] The coverage don't support clang or MacOS"
	exit 0
fi
