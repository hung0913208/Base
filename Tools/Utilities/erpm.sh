#!/bin/bash

PRIMARY=""
DEBUG=0
ROOT=""
URL=""

# @NOTE: print log info
info(){
	if [ $# -eq 2 ]; then
		echo "[   INFO  ]: $1 line ${SCRIPT}:$2"
	else
		echo "[   INFO  ]: $1"
	fi
}

# @NOTE: print log warning
warning(){
	if [ $# -eq 2 ]; then
		echo "[ WARNING ]: $1 line ${SCRIPT}:$2"
	else
		echo "[ WARNING ]: $1"
	fi
}

# @NOTE: print log error and exit immediatedly
error(){
	if [ $# -eq 2 ]; then
		echo "[  ERROR  ]: $1 line ${SCRIPT}:$2"
	else
		echo "[  ERROR  ]: $1"
	fi
	set +x
	exit -1
}

function version() {
	SAVE=$IFS; IFS=$'\n';
	LINE=""
	
	for RAW in $(grep -n "<name>.*</name>" $PRIMARY | grep "$1"); do
		if [ $(echo $RAW | awk '{split($0,a,"name>"); print a[2]}' | awk '{split($0,a,"</name"); print a[1]}') = $1 ]; then
			LINE=$(echo $RAW | awk '{split($0,a,":"); print a[1]}')	

			for VER in $(grep -n "<version .* ver=\".*\" .*/>" $PRIMARY); do
				if [[ $LINE -gt $(echo $VER | awk '{split($0,a,":"); print a[1]}') ]]; then
					echo $VER | awk '{split($0,a,"ver=\""); print a[2]}' | awk '{split($0,a,"\""); print a[1]}'
					break
				fi
			done
		fi
	done

	IFS=$SAVE	
}

function location() {
	SAVE=$IFS; IFS=$'\n';
	LINE=""
	
	for RAW in $(grep -n "<name>.*</name>" $PRIMARY | grep "$1"); do
		if [ $(echo $RAW | awk '{split($0,a,"<name>"); print a[2]}' | awk '{split($0,a,"</name>"); print a[1]}') = $1 ]; then
			LINE=$(echo $RAW | awk '{split($0,a,":"); print a[1]}')	
			break
		fi
	done

	if [[ ${#LINE} -gt 0 ]]; then	
		for LOC in $(grep -n "<location href=.*.rpm\"/>" $PRIMARY); do
			if [[ $LINE -lt $(echo $LOC | awk '{split($0,a,":"); print a[1]}') ]]; then
				echo $LOC | awk '{split($0,a,"\""); print a[2]}'
				break
			fi
		done
	fi
	IFS=$SAVE
}

function package() {
	SAVE=$IFS; IFS=$'\n';
	NAME=""
	LINE=""	

	if echo "$1" | grep "<=" >& /dev/null; then
		REQUEST=$(echo $1 | awk '{split($0,a,"<="); print a[1]}' | xargs)
		VERSION=$(echo $1 | awk '{split($0,a,"<="); print a[2]}' | xargs)
	elif echo "$1" | grep ">=" >& /dev/null; then
		REQUEST=$(echo $1 | awk '{split($0,a,">="); print a[1]}' | xargs)
		VERSION=$(echo $1 | awk '{split($0,a,">="); print a[2]}' | xargs)
	elif echo "$1" | grep "<" >& /dev/null; then
		REQUEST=$(echo $1 | awk '{split($0,a,">"); print a[1]}' | xargs)
		VERSION=$(echo $1 | awk '{split($0,a,">"); print a[2]}' | xargs)
	elif echo "$1" | grep ">" >& /dev/null; then
		REQUEST=$(echo $1 | awk '{split($0,a,"<"); print a[1]}' | xargs)
		VERSION=$(echo $1 | awk '{split($0,a,"<"); print a[2]}' | xargs)
	elif echo "$1" | grep "=" >& /dev/null; then
		REQUEST=$(echo $1 | awk '{split($0,a,"="); print a[1]}' | xargs)
		VERSION=$(echo $1 | awk '{split($0,a,"="); print a[2]}' | xargs)
	else
		REQUEST=$(basename $1)
	fi

	for ENTRY in $(grep -n "<rpm:entry name=\"$REQUEST\"" $PRIMARY | awk '{split($0,a,":"); print a[1]}'); do
		LINE=$ENTRY
		PROVIDE=""
		REQUIRE=""

		for ITEM in ${PROVIDES[@]}; do
			if [[ $ITEM -lt $LINE ]]; then
				PROVIDE=$ITEM
			else
				break
			fi
		done

		for ITEM in ${REQUIRES[@]}; do	
			if [[ $ITEM -lt $LINE ]]; then
				REQUIRE=$ITEM
			else
				break
			fi
		done

		if [[ $PROVIDE -gt $REQUIRE ]]; then
			break
		else
			LINE=""
		fi
	done

	if [[ ${#LINE} -gt 0 ]]; then
		for PACKAGE in $(grep -n "<location href=.*.rpm\"/>" $PRIMARY); do
			if [[ $LINE -gt $(echo $PACKAGE | awk '{split($0,a,":"); print a[1]}') ]]; then
				NAME=$(echo $PACKAGE | awk '{split($0,a,"\""); print a[2]}')
			else
				break
			fi
		done

		echo $NAME
	fi
	IFS=$SAVE
}

function install() {
	for PKG in $@; do
		LOC=$(location $PKG)

		if [[ ${#LOC} -gt 0 ]]; then
			if [ ! -f $ROOT/$(basename $LOC) ]; then
				curl -k -o "$ROOT/$(basename $LOC)" $URL/$LOC
			fi
		else
			error "not found $PKG"
		fi
		
		DONE=()
		IGNORED=()

		while [ 1 ]; do
			HAVING=$(ls -1q $ROOT/ | wc -l)

			for RPM in "$ROOT/*.rpm"; do
				SAVE=$IFS; IFS=$'\n';
				FOUND=0
				PROVIDES=($(rpm -qp --provides --nosignature $RPM))

				# @NOTE: check if this RPM has been done
				for ITEM in ${DONE[@]}; do
					if [ "$RPM" = "$ITEM" ]; then
						FOUND=1
						break
					fi
				done
					
				if [[ $FOUND -eq 1 ]]; then
					continue
				fi	

				for LINE in $(rpm -i --nosignature $RPM 2>&1 | tail -n +4); do
					FOUND=0

					if echo "$LINE" | grep "<=" >& /dev/null; then
						REQUEST=$(echo $LINE | awk '{split($0,a,"<="); print a[1]}' | xargs)
					elif echo "$LINE" | grep ">=" >& /dev/null; then
						REQUEST=$(echo $LINE | awk '{split($0,a,">="); print a[1]}' | xargs)
					elif echo "$LINE" | grep "<" >& /dev/null; then
						REQUEST=$(echo $LINE | awk '{split($0,a,">"); print a[1]}' | xargs)
					elif echo "$LINE" | grep ">" >& /dev/null; then
						REQUEST=$(echo $LINE | awk '{split($0,a,"<"); print a[1]}' | xargs)
					elif echo "$LINE" | grep "=" >& /dev/null; then
						REQUEST=$(echo $LINE | awk '{split($0,a,"="); print a[1]}' | xargs)
					else
						REQUEST=$(basename $LINE)
					fi

					for ITEM in ${IGNORED[@]}; do
						if echo $ITEM | grep $REQUEST >& /dev/null; then
							FOUND=1
							break
						fi
					done

					if [[ $FOUND -eq 0 ]]; then
						for LOADED  in "$ROOT/*.rpm"; do
							for ITEM in $(rpm -qp --provides --nosignature $LOADED); do
								if echo $ITEM | grep $REQUEST >& /dev/null; then
									FOUND=1
									break
								fi
							done

							if [[ $FOUND -eq 1 ]]; then
								break
							fi
						done
					fi

					# @NOTE: check if this depenency was itself
					if [[ $FOUND -eq 0 ]]; then
						for ITEM in ${PROVIDES[@]}; do
							if echo $ITEM | grep $REQUEST >& /dev/null; then
								FOUND=1
								break
							fi
						done
					fi

					# @NOTE: answer the question, do we need to install this RPM?
					if [[ $FOUND -eq 0 ]]; then
						LOC=$(package "$LINE")
						DONE="${DONE[@]} $(basename $LOC)"
						IGNORED="${IGNORED[@]} $REQUEST"

						if [[ ${#LOC} -eq 0 ]]; then
							warning "not found depenency '$LINE' of package $PKG"
						elif [ ! -f "$ROOT/$(basename $LOC)" ]; then
							curl -k -o "$ROOT/$(basename $LOC)" $URL/$LOC
						fi
					fi
				done
				IFS=$SAVE
			done
			
			if [[ $HAVING -eq $(ls -1q $ROOT | wc -l) ]]; then
				break
			fi
		done
	done
	for RPM in "$CACHE/*.rpm"; do
		exec 2>&-

		if ! rpm -ivh --nosignature $1; then
			SAVE=$IFS; IFS=$'\n';
			exec 2>&1

			for LINE in $(rpm -i --nosignature $RPM 2>&1 | tail -n +4); do
				PACKAGE=$(package $(echo $LINE | awk '{split($0,a," is needed by "); print a[1]'))

				if [ ! -f "$CACHE/$(basename $PACKAGE)" ]; then
					curl -k -o "$ROOT/$(basename $PACKAGE)" $PACKAGE
					ln -s $ROOT/$(basename $PACKAGE) $CACHE/$(basename $PACKAGE)
				fi
			done
			IFS=$SAVE
		fi
	done
}

function fetch() {	
	for PKG in $@; do
		LOC=$(location $PKG)

		if [[ ${#LOC} -gt 0 ]]; then
			if [ ! -f $ROOT/$(basename $LOC) ]; then
				curl -k -o "$ROOT/$(basename $LOC)" $URL/$LOC
			fi
		else
			error "not found $PKG"
		fi
		
		DONE=()
		IGNORED=()

		while [ 1 ]; do
			HAVING=$(ls -1q $ROOT/ | wc -l)

			for RPM in "$ROOT/*.rpm"; do
				SAVE=$IFS; IFS=$'\n';
				FOUND=0
				PROVIDES=($(rpm -qp --provides --nosignature $RPM))

				# @NOTE: check if this RPM has been done
				for ITEM in ${DONE[@]}; do
					if [ "$RPM" = "$ITEM" ]; then
						FOUND=1
						break
					fi
				done
					
				if [[ $FOUND -eq 1 ]]; then
					continue
				fi	

				for LINE in $(rpm -qp --requires --nosignature $RPM); do
					FOUND=0

					if echo "$LINE" | grep "<=" >& /dev/null; then
						REQUEST=$(echo $LINE | awk '{split($0,a,"<="); print a[1]}' | xargs)
					elif echo "$LINE" | grep ">=" >& /dev/null; then
						REQUEST=$(echo $LINE | awk '{split($0,a,">="); print a[1]}' | xargs)
					elif echo "$LINE" | grep "<" >& /dev/null; then
						REQUEST=$(echo $LINE | awk '{split($0,a,">"); print a[1]}' | xargs)
					elif echo "$LINE" | grep ">" >& /dev/null; then
						REQUEST=$(echo $LINE | awk '{split($0,a,"<"); print a[1]}' | xargs)
					elif echo "$LINE" | grep "=" >& /dev/null; then
						REQUEST=$(echo $LINE | awk '{split($0,a,"="); print a[1]}' | xargs)
					else
						REQUEST=$(basename $LINE)
					fi

					for ITEM in ${IGNORED[@]}; do
						if echo $ITEM | grep $REQUEST >& /dev/null; then
							FOUND=1
							break
						fi
					done

					if [[ $FOUND -eq 0 ]]; then
						for LOADED  in "$ROOT/*.rpm"; do
							for ITEM in $(rpm -qp --provides --nosignature $LOADED); do
								if echo $ITEM | grep $REQUEST >& /dev/null; then
									FOUND=1
									break
								fi
							done

							if [[ $FOUND -eq 1 ]]; then
								break
							fi
						done
					fi

					# @NOTE: check if this depenency was itself
					if [[ $FOUND -eq 0 ]]; then
						for ITEM in ${PROVIDES[@]}; do
							if echo $ITEM | grep $REQUEST >& /dev/null; then
								FOUND=1
								break
							fi
						done
					fi

					# @NOTE: answer the question, do we need to install this RPM?
					if [[ $FOUND -eq 0 ]]; then
						LOC=$(package "$LINE")
						IGNORED="${IGNORED[@]} $REQUEST"

						if [[ ${#LOC} -eq 0 ]]; then
							warning "not found depenency '$LINE' of package $PKG"
						elif [ ! -f "$ROOT/$(basename $LOC)" ]; then
							DONE="${DONE[@]} $(basename $LOC)"
							curl -k -o "$ROOT/$(basename $LOC)" $URL/$LOC
						fi
					fi
				done
				IFS=$SAVE
			done
			
			if [[ $HAVING -eq $(ls -1q $ROOT | wc -l) ]]; then
				break
			fi
		done
	done
}

CMD=$1
shift

while [ $# -gt 0 ]; do
	case $1 in
		--root)		ROOT="$2"; shift;;
		--primary)	PRIMARY="$2"; shift;;
		--verbose) 	set -x;;
		--debug)	DEBUG=1;;
		(--) 		shift; break;;
		(-*) 		error "unrecognized option $1";;
		(*) 		break;;
	esac

	shift
done

if [[ ${#PRIMARY} -gt 0 ]]; then
	URL=$(dirname $(dirname $PRIMARY))

	if [[ $PRIMARY =~ "https://"* ]] || [[ $PRIMARY =~ "http://"* ]]; then
		if [ -f "$(pwd)/$(basename $PRIMARY)" ]; then
			GZPRIMARY="$(pwd)/$(basename $PRIMARY)"
		elif curl -kO $PRIMARY >& /dev/null; then
			GZPRIMARY="$(pwd)/$(basename $PRIMARY)"
		elif [ ! -f "$(pwd)/$(basename $PRIMARY)" ]; then
			error "can't fetch $PRIMARY"
		else
			GZPRIMARY="$(pwd)/$(basename $PRIMARY)"
		fi
		PRIMARY=${GZPRIMARY%.*}

		if [ ! -f $PRIMARY ]; then
			gzip $GZPRIMARY > $PRIMARY
		fi
	fi

	PROVIDES=($(grep -n '<rpm:provides>' $PRIMARY | awk '{split($0,a,":"); print a[1]}'))
	REQUIRES=($(grep -n '<rpm:requires>' $PRIMARY | awk '{split($0,a,":"); print a[1]}'))

	case $CMD in
		fetch) 		fetch $@;;
		install) 	install $@;;
		(*)		break;;
	esac
else
	error "please provide primary url before doing anything"
fi

set +x
