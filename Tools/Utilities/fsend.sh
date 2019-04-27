#!/bin/bash

upload() {
	# @NOTE: get unique key
	key=$(curl -sS --header "Content-Type: application/json" \
			--request POST \
			--data "{}" \
			http://fsend.vn/v2/up-keys | \
		python -c "import sys, json; print(json.load(sys.stdin)['key'])")

	# @NOTE: get session info
	session=$(curl -sS --header "Content-Type: application/json" \
			--request PUT \
			--data "{\"file_name\":\"$(basename "$1")\",\"file_size\":$(wc -c < "$1")}" \
			http://fsend.vn/v2/up-keys/$key/upload)

	# @NOTE: generate stream to upload
	stream=$(echo $session | python -c "import sys, json; print(json.load(sys.stdin)['location'])")

	# @NOTE: upload file to fsend server
	curl --progress-bar -X POST -o result.txt -T "$1" $stream

	if [ $? == 0 ]; then
		# @NOTE: remove result.txt since we finish uploading
		rm -fr result.txt

		# @NOTE: notify to client using email
		if [ $# == 2 ]; then
			message="{\"recipients\":[\"$2\"],\"message\":\"$1\",\"title\":null,\"password_lock\":null}"
			curl -sS --header "Content-Type: application/json" \
				--request POST \
				--data $message \
				http://fsend.vn/v2/transfers?key=$key >& /dev/null
		fi
	fi
}

download() {
	key=$(basename $1)

	# @NOTE: get session info
	session=$(curl -sS --header "Content-Type: application/json" \
			--request PUT \
			--data "{\"file_name\":\"$2\"}" \
			http://fsend.vn/v2/transfers/$key/download)

	# @NOTE: get url from session
	url=$(echo $session | python -c "import sys, json; print(json.load(sys.stdin)['location'])")

	# @NOTE: download url
	curl $url -o $3
}

# @NOTE: some files are very big and we must split them to several file for uploading
if [ $1 == "upload" ]; then
	if [ ! -e $2 ]; then
		exit -1
	elif [ $(wc -c < $2) > 2147483648 ]; then
		NAME=$(basename $2)
		EXT=${NAME##*.}
		ROOT=$(pwd)

		# @NOTE: remove dir $ROOT/tmp
		if [ -d $ROOT/tmp ]; then
			rm -fr $ROOT/tmp
		fi

		# @NOTE: jump to working dir
		mkdir $ROOT/tmp && cd $ROOT/tmp

		# @NOTE: bug on fsend.vn causes me to split big files into several
		# small files
		split -b 2G "$2" "$NAME."

		# @NOTE: upload small files to fsend.vn
		if [ $? == 0 ]; then
			for SPLITED in $ROOT/tmp/*; do
				mv "$SPLITED" "$SPLITED.$EXT"

				if [ $? == 0 ]; then
					upload "$SPLITED.$EXT" "$3"
				else
					exit -1
				fi
			done
		else
			exit -1
		fi

		# @NOTE: back to root dir
		cd $ROOT && rm -fr tmp
	else
		upload $2 $3
	fi
elif [ $1 == "download" ]; then
	if [ $# == 3 ]; then
		ROOT=$(pwd)

		if [ ! -f $2 ]; then
			exit -1
		fi

		# @NOTE: clear everything inside $ROOT/tmp first
		if [ ! -e $ROOT/tmp ]; then
			rm -fr $ROOT/tmp
		fi

		# @NOTE: jump to $ROOT/tmp
		mkdir -p $ROOT/tmp && cd $ROOT/tmp

		# @NOTE: download everything into $ROOT/tmp
		cat "$2" | while read DEFINE; do
			SPLITED=($(echo "$DEFINE" | tr ' ' '\n'))
			LINK=${SPLITED[0]}
			INPUT=${SPLITED[1]}
			OUTPUT=${SPLITED[2]}

			download $LINK $INPUT $(pwd)/$OUTPUT
		done

		# @NOTE: combine everything into a single one file
		cat $ROOT/tmp/* >> $3

		# @NOTE: back to $ROOT
		cd $ROOT && rm -fr $ROOT/tmp
	else
		download $2 $3 $4
	fi
fi
