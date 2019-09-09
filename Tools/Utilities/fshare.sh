#!/bin/bash

SERVING="https://api.fshare.vn"
TOKEN=""
SESSION=""
AVAILABLE=0

function update() {
	AVAILABLE=$(curl -sS --cookie /tmp/fshare.cookies	\
			--request GET 				\
			$SERVING/api/user/get |
			python -c """
import sys, json

resp = json.load(sys.stdin)

print(int(resp['webspace']) - int(resp['webspace_used']))
""")
}

function login() {
	APP="L2S7R6ZMagggC5wWkQhX2+aDi467PPuftWUMRFSn"
	FORM="{\"app_key\":\"$APP\",\"user_email\":\"$1\",\"password\":\"$2\"}"

	RESP=$(curl -sS --request POST -c /tmp/fshare.cookies		\
			--header "Content-Type: application/json" 	\
			--data "$FORM" 					\
			$SERVING/api/user/login)

	TOKEN=$(python -c """
import sys, json

resp = json.loads('$RESP')
if resp['code'] != 200:
	sys.exit(-1)
else:
	print(resp['token'])
""")

	SESSION=$(python -c """
import sys, json

resp = json.loads('$RESP')
if resp['code'] != 200:
	sys.exit(-1)
else:
	print(resp['session_id'])
""")

	if [[ ${#SESSION} -eq 0 ]] || [[ ${#TOKEN} -eq 0 ]]; then
		return 1
	else
		update
		return $?
	fi
}

function list() {
	RESULT=()
	IDX=0
	DIR=$1

	if [ ! -f /tmp/fshare.cookies ]; then
		return 1
	fi

	while true; do
		ARRAY=$(curl -sS --request GET 			\
				--cookie /tmp/fshare.cookies 	\
				"$SERVING/api/fileops/list$DIR?pageIndex=$IDX" |
			python -c """
import json, sys
resp = json.load(sys.stdin)
for item in resp:
	if 'name' in item:
		print(item['name'])
""")

		if [[ "${#ARRAY[@]}" -gt 0 ]]; then
			RESULT=("${RESULT[@]}" "${ARRAY[@]}")
			IDX=$((IDX+1))
		else
			break
		fi
	done

	echo "${RESULT[@]}"
}

function mkfolder() {
	FORM="{\"token\":\"$TOKEN\",\"name\":\"$(basename $1)\",\"in_dir\":\"$(dirname $1)\"}"

	echo $FORM
	if [[ ${#TOKEN} -eq 0 ]]; then
		return 1
	fi

	if ! curl -sS --request POST --data "$FORM" 			\
			--header "Content-Type: application/json" 	\
			--cookie /tmp/fshare.cookies 			\
			$SERVING/api/fileops/createFolderInPath |
	python """
import json, sys
resp = json.load(sys.stdin)

if resp.get('code') != 200:
	sys.exit(-1)
"""; then
		return 1
	fi
}

function remove() {
	FORM="{\"token\":\"$TOKEN\",\"items\":[\"$1\"]}"

	if [[ ${#TOKEN} -eq 0 ]]; then
		return 1
	fi

	if curl -sS --request POST --data "$FORM" 		\
		--header "Content-Type: application/json" 	\
		--cookie /tmp/fshare.cookies 			\
		$SERVING/api/fileops/delete | python -c """
import json, sys
resp = json.load(sys.stdin)

if resp.get('code') != 200:
	sys.exit(-1)
"""; then
		update
	else
		return 1
	fi
}

function upload() {
	SIZE=$(wc -c $1 | awk '{ print $1}')
	NAME=$(basename $1)
	RPATH=$2
	FORM="{\"token\":\"$TOKEN\",\"name\":\"$NAME\",\"path\":\"$RPATH\",\"secured\":1,\"size\":\"$SIZE\"}"

	if [ ! -f /tmp/fshare.cookies ]; then
		return 1
	elif [ $SIZE -gt $AVAILABLE ]; then
		return 1
	elif [[ ${#TOKEN} -eq 0 ]]; then
		return 1
	fi

	LOCATION=$(curl -sS --header "Content-Type: application/json" 	\
		--cookie /tmp/fshare.cookies 			\
		--request POST 					\
		--data "$FORM" 					\
		$SERVING/api/Session/upload |
	python -c """
import sys, json

resp = json.load(sys.stdin)
if resp['code'] != 200:
	sys.exit(-1)
else:
	print(resp['location'])
""")

	if [[ ${#LOCATION} -eq 0 ]]; then
		return 1
	fi

	if ! curl --progress-bar 			\
			-H "Content-Type: $TYPE" 	\
			--request POST 			\
			--data-binary "@$1" $LOCATION; then
		update
	fi
}

