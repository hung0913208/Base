#!/bin/bash

function sha256() {
	echo -n $1 | shasum -a 256 | awk '{ print $1 }'
}

function login() {
	if [ ! -f /tmp/upfile.cookies ]; then
		SHA=$(sha256 "$(sha256 'UpFile.VN')$2" | awk '{ print toupper($0) }')

		$CURL -sS --request POST -c /tmp/upfile.cookies 	\
			--data "Act=Login&Email=$1&Password=$SHA" 	\
			"http://upfile.vn/" |
		python -c """
import sys, json

try:
	resp = json.load(sys.stdin)

	if resp.get('Status') != True:
		sys.exit(-1)
except Exception:
	sys.exit(-1)
"""
	fi
}

function upload() {
	HASH=$(sha256 $(basename $1))
	SALT=$(sha256 "$(wc -c < "$1")")

	DATA=($($CURL -sS --request POST -c /tmp/upfile.cookies 	\
		--cookie /tmp/upfile.cookies 				\
		--data "Act=Checksum&Hash=$HASH&Salt=$SALT" 		\
		"http://upfile.vn/" |
	python -c """
import sys, json

try:
	raw = ''

	for c in sys.stdin.read():
		if 0 <= ord(c) and ord(c) <= 125:
			raw += c
	else:
		resp = json.loads(raw)

	if resp.get('Status') != True:
		sys.exit(-1)
	else:
		print(resp['Data'][0])
		print(resp['Data'][1])
except Exception as error:
	print(error)
	sys.exit(-1)
"""))

	if [ $? != 0 ]; then
		return -1
	fi
	echo ${DATA[0]}
	echo ${DATA[1]}
	DATA0=$(python -c "print('${DATA[0]}'.lower()[7:15])")
	DATA1=$(python -c "print('${DATA[1]}'.lower()[7:15])")
	DATA2=$(python -c "print('$(sha256 "${DATA[0]}""${DATA[1]}")'.upper()[7:20])")

	URL="http://upfile.vn/$DATA0/$DATA1/$(sha256 "$DATA0""$DATA1""UpFile.VN")/"

	curl -F "File[]=@\"$1\"" -F "Hash=$DATA2" --cookie /tmp/upfile.cookies 		\
		--header 'Origin: http://upfile.vn' 			\
		--header 'Referer: http://upfile.vn/filemanager/' 	\
		$URL
}

function list() {
	if [[ $# -gt 0 ]]; then
		SUB=$1
		PARENT=$SUB

		if [ $SUB = '/' ]; then
			SUB='0'
		fi
	else
		SUB="0"
		PARENT='/'
	fi

	if [[ $# -gt 1 ]]; then
		FILE="$2"
	else
		FILE=''
	fi

	if [[ $# -gt 2 ]]; then
		FOLDER="$3"
	else
		FOLDER=''
	fi

	HASH=$(python -c "print('$(sha256 "$SUB""UpFile.VN")'.upper()[7:15])")
	$CURL -sS --request POST -c /tmp/upfile.cookies --cookie /tmp/upfile.cookies 	\
		--header 'Content-Type: application/x-www-form-urlencoded; charset=UTF-8'	\
		--header 'Accept: application/json, text/javascript, */*; q=0.01' 		 	\
		--data "Act=getListItem&Sub=$SUB&Hash=$HASH" 							 	\
		"http://upfile.vn/filemanager/" |
 	python -c """
import json, sys, pprint
try:
	raw = ''

	for c in sys.stdin.read():
		if 0 <= ord(c) and ord(c) <= 125:
			raw += c
	else:
		resp = json.loads(raw)

	modes = '$FOLDER'.split('|')
	if len(modes) > 1 or modes[0] != '':
		for item in resp['Folder']:
			result = ''

			for mode in modes:
				if len(result) == 0:
					result = item[mode]
				else:
					result += ' {}'.format(item[mode])
			else:
				print(result)
	else:
		for item in resp['Folder']:
			print('{}{}/'.format('$PARENT', item['folderName']))

	modes = '$FILE'.split('|')
	if len(modes) > 1 or modes[0] != '':
		for item in resp['File']:
			result = ''

			for mode in modes:
				if len(result) == 0:
					result = item[mode]
				else:
					result += '|{}'.format(item[mode])
			else:
				print(result)
	else:
		for item in resp['File']:
			print('{}{}'.format('$PARENT', item['Name']))
except Exception as error:
	sys.exit(-1)
"""
}

function delete() {
	TYPE=$1
	IDs=$2

	ID=$(python -c "print('-'.join('$IDs'.split(';')))")
	LENGTH=$(python -c "print(len('$IDs'.split(';')))")

	if [ $TYPE = 'File' ]; then
		HASH=$(sha256 "$LENGTH""$ID""0" | awk '{ print toupper($0) }')
	else
		HASH=$(sha256 "0""$LENGTH""$ID" | awk '{ print toupper($0) }')
	fi

	LIST=$(python -c """
result = ''

for id in '$IDs'.split(';'):
	if len(result) == 0:
		result = '$TYPE[]={}'.format(id)
	else:
		result += '&$TYPE[]={}'.format(id)
else:
	print(result)
""")

	$CURL -sS --request POST -c /tmp/upfile.cookies --cookie /tmp/upfile.cookies 	\
		--data "Act=deleteItem&$LIST&Hash=$HASH" 				\
		http://upfile.vn/filemanager/
}

function delete_all_files() {
	SAVE=$IFS
	IFS=$'\n'
	LIST=$(list / '' 'ID')

	for ITEM in ${LIST[@]}; do
		IFS=$SAVE
		ID=$(echo $ITEM | awk '{ split($0,a,"|"); print(a[2])}')

		if [[ ${#ID} -gt 1 ]]; then
			delete "File" $ID
		fi

		SAVE=$IFS
		IFS=$'\n'
	done
	IFS=$SAVE
}

function delete_all_folders() {
	SAVE=$IFS
	IFS=$'\n'
	LIST=$(list / '' 'folderName|ID')

	for ITEM in ${LIST[@]}; do
		IFS=$SAVE
		ID=$(echo $ITEM | awk '{ split($0,a,"|"); print(a[2])}')

		if [[ ${#ID} -gt 1 ]]; then
			delete "Folder" $ID
		fi

		SAVE=$IFS
		IFS=$'\n'
	done
	IFS=$SAVE
}

CMD=$1
shift

case $CMD in
	upload)
		while [ $# -gt 0 ]; do
			case $1 in
				--proxy) 	PROXY="$2"; shift;;
				--file) 	FILE="$2"; shift;;
				--email) 	EMAIL="$2"; shift;;
				--password) 	PASSWORD="$2"; shift;;
				(--) 		shift; break;;
				(-*) 		error "unrecognized option $1";;
				(*) 		break;;
			esac
			shift
		done

		if [[ ${#PROXY} -gt 0 ]]; then
			CURL="$(which curl) -x $PROXY"
		else
			CURL="$(which curl)"
		fi

		if [[ ${#EMAIL} -eq 0 ]]; then
			error "please provide email"
		fi

		if [[ ${#PASSWORD} -eq 0 ]]; then
			error "please provide password"
		fi

		if [[ ${#FILE} -eq 0 ]]; then
			error "please provide file for upload"
		fi

		if ! login $EMAIL $PASSWORD; then
			error "Login failed"
		fi

		upload $FILE
		;;
	clean)
		while [ $# -gt 0 ]; do
			case $1 in
				--proxy) 	PROXY="$2"; shift;;
				--email) 	EMAIL="$2"; shift;;
				--password) 	PASSWORD="$2"; shift;;
				(--) 		shift; break;;
				(-*) 		error "unrecognized option $1";;
				(*) 		break;;
			esac
			shift
		done

		if [[ ${#PROXY} -gt 0 ]]; then
			CURL="$(which curl) -x $PROXY"
		else
			CURL="$(which curl)"
		fi

		if [[ ${#EMAIL} -eq 0 ]]; then
			error "please provide email"
		fi

		if [[ ${#PASSWORD} -eq 0 ]]; then
			error "please provide password"
		fi

		if ! login $EMAIL $PASSWORD; then
			error "Login failed"
		fi

		if [[ ${#TYPE} -eq 0 ]]; then
			error "please provide type of $NAME"
		fi

		delete_all_files
		delete_all_folders
		;;
esac

