#!/bin/bash

WEBSOCKET="$(dirname $0)/../Websocket/Websocket.sh"
BUILD=$(python -c "print('$2'.split('.')[0])")
JOB=$(python -c "print('$2'.split('.')[-1])")

function error() {
	if [ $# -eq 2 ]; then
		echo "[  ERROR  ]: $1 line ${SCRIPT}:$2"
	else
		echo "[  ERROR  ]: $1 in ${SCRIPT}"
	fi
	exit -1
}

function log() {
	PROTOCOL="7"
	VESION="5.0.0"
	$WEBSOCKET "wss://ws.pusherapp.com/app/$PUSHER?protocol=$PROTOCOL&client=js&version=$VERSION&flash=false" << EOF
EOF
}

function repository() {
	REPO=$(python -c """
try:
    from urllib import quote_plus
except ImportError:
    from urllib.parse import quote_plus
print(quote_plus('$1'))
""")

	curl -sS --request GET 				\
		--header "Authorization: token $TOKEN" 	\
		--header "Travis-API-Version: 3" 	\
		"https://api.travis-ci.org/repo/$REPO" |
	python -c """
from pprint import pprint
from json import load
from sys import stdin

print(load(stdin)['id'])
	"""
}

function history() {
	KEEP=1
	EVENT='push%2Capi%2Ccron'
	OFFSET=0

	while [[ $KEEP -eq 1 ]]; do
		RESP=$(curl -sS --request GET 				\
				--header "Authorization: token $TOKEN" 	\
				--header "Travis-API-Version: 3" 	\
			"https://api.travis-ci.org/repo/$REPO/builds?event_type=$EVENT&repository_id=$REPO&offset=$OFFSET")


		if [[ ${#LIMIT} -eq 0 ]]; then
			LIMIT=$(echo $RESP | python -c """
import json, sys
print(json.load(sys.stdin)['@pagination']['limit'])
""")
		fi

		if [[ ${#LAST} -eq 0 ]]; then
			LAST=$(echo $RESP | python -c """
import json, sys
print(json.load(sys.stdin)['builds'][0]['number'])
""")
		fi

		if [[ ${#IDX} -eq 0 ]]; then
			IDX=$LAST
		fi

		echo $RESP | python -c """
import json, sys

for item in json.load(sys.stdin)['builds']:
	print(item['id'])
		"""

		IDX=$((IDX-LIMIT))
		OFFSET=$((OFFSET+LIMIT))

		if [[ $IDX -gt 0 ]]; then
			KEEP=1
		else
			KEEP=0
		fi
	done
}

function jobs() {
	ID=$1
	RESP=$(curl -sS --request GET 				\
			--header "Authorization: token $TOKEN" 	\
			--header "Travis-API-Version: 3" 	\
			https://api.travis-ci.org/build/$ID/jobs)

	echo $RESP | python -c """
import json, sys

for item in json.load(sys.stdin)['jobs']:
	print(item['id'])
"""
}

function restart() {
	if [ $1 = 'build' ]; then
		RESP=$(curl -sS --request POST 				\
				--header "Authorization: token $TOKEN" 	\
				--header "Travis-API-Version: 3" 	\
			https://api.travis-ci.org/$1/$2/restart)
	else
		RESP=$(curl -sS --request POST 								\
				--header "Authorization: token $TOKEN" 					\
				--header "Travis-API-Version: 3" 					\
				--header "Referer: https://travis-ci.org/hung0913208/build/builds/$3" 	\
			https://api.travis-ci.org/$1/$2/restart)
	fi

	echo $RESP | python -c """
import json, sys

try:
	if json.load(sys.stdin).get('state_change') != 'restart':
		sys.exit(-1)
except Exception as error:
	sys.exit(-1)
"""
}

function build() {
	ID=$1
	KEEP=1
	EVENT='push%2Capi%2Ccron'
	OFFSET=0

	while [[ $KEEP -eq 1 ]]; do
		RESP=$(curl -sS --request GET 				\
				--header "Authorization: token $TOKEN" 	\
				--header "Travis-API-Version: 3" 	\
			"https://api.travis-ci.org/repo/$REPO/builds?event_type=$EVENT&repository_id=$REPO&offset=$OFFSET")


		if [[ ${#LIMIT} -eq 0 ]]; then
			LIMIT=$(echo $RESP | python -c """
import json, sys
print(json.load(sys.stdin)['@pagination']['limit'])
""")
		fi

		if [[ ${#LAST} -eq 0 ]]; then
			LAST=$(echo $RESP | python -c """
import json, sys
print(json.load(sys.stdin)['builds'][0]['number'])
""")
		fi

		if [[ ${#IDX} -eq 0 ]]; then
			IDX=$LAST
		fi

		if [[ $1 -le $IDX ]]; then
			ADJUST=$((IDX-ID))

			echo $RESP | python -c """
import json, sys

for idx, item in enumerate(json.load(sys.stdin)['builds']):
	if idx == $ADJUST:
		print(item['id'])
		break
			"""
			break
		fi

		IDX=$((IDX-LIMIT))
		OFFSET=$((OFFSET+LIMIT))

		if [[ $IDX -gt 0 ]]; then
			KEEP=1
		else
			KEEP=0
		fi
	done
}

function status() {
	ID=$1
	RESP=$(curl -sS --request GET 				\
			--header "Authorization: token $TOKEN" 	\
			--header "Travis-API-Version: 3" 	\
			https://api.travis-ci.org/build/$ID/jobs)

	echo $RESP | python -c """
import json, sys

for idx, item in enumerate(json.load(sys.stdin)['jobs']):
	if idx + 1 == $2:
		print(item['state'])
		break
"""
}

function job() {
	ID=$1
	RESP=$(curl -sS --request GET 				\
			--header "Authorization: token $TOKEN" 	\
			--header "Travis-API-Version: 3" 	\
			https://api.travis-ci.org/build/$ID/jobs)

	echo $RESP | python -c """
import json, sys

for idx, item in enumerate(json.load(sys.stdin)['jobs']):
	if idx + 1 == $2:
		print(item['id'])
		break
"""
}

if [ $1 = 'restart' ] || [ $1 = 'status' ]; then
	TASK=$1
	shift
	
	if ! options=$(getopt -l token,patch,job,repo: -- "$@")
	then
		error "Can' parse $BASE/Tools/Builder/build $@"
	fi

	while [ $# -gt 0 ]; do
		case $1 in
			--job)		JOB="$2"; shift;;
			--repo)		REPO="$2"; shift;;
			--build)	BUILD="$2"; shift;;
			--token)	TOKEN="$2" ; shift;;
			(--) 		shift; break;;
			(-*) 		error "unrecognized option $1";;
			(*) 		break;;
		esac
		shift
	done

	if [[ ${#TOKEN} -eq 0 ]]; then
		error "please provide token before doing anything"
	elif [[ ${#REPO} -eq 0 ]]; then
		error "please provide repo before doing anything"
	fi

	REPO=$(repository $REPO)
	BUILD=$(build $BUILD)

	if [ $TASK = 'status' ]; then
		if [[ ${#JOB} -ne 0 ]]; then
			status $BUILD $JOB
		fi
	elif [[ ${#JOB} -eq 0 ]]; then
		if ! restart 'build' $BUILD; then
			error "Can't restart travis"
		fi
	else
		JOB=$(job $BUILD $JOB)

		if restart 'job' $JOB $BUILD; then
			echo ''
		fi
	fi
elif [ $1 = 'env' ]; then
	shift

	if [ $1 = 'set' ]; then
		shift
	fi
fi
