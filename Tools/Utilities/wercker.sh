#!/bin/bash

WEBSOCKET="$(dirname $0)/../Websocket/Websocket.sh"
TOKEN=''

function error() {
	if [ $# -eq 2 ]; then
		echo "[  ERROR  ]: $1 line ${SCRIPT}:$2"
	else
		echo "[  ERROR  ]: $1 in ${SCRIPT}"
	fi
	exit -1
}

function heartbeat() {
	curl -sS --request GET --header "Cookie: express.sid=$TOKEN" \
		"https://app.wercker.com/heartbeat?_=$(date -u)" |
	python -c "import sys, json; sys.exit(json.load(sys.stdin)['loggedIn'] != true)"
	return $?
}

function pipeline() {
	curl -sS --request GET --header "Cookie: express.sid=$TOKEN" \
		"https://app.wercker.com/api/v3/applications/hung0913208/build/pipelines?limit=$2" |
	python -c """
import sys, json

for pipe in json.load(sys.stdin):
	if pipe['name'] == '$1':
		print(pipe['id'])
		break
"""
}

function workflows() {
	curl -sS --request GET --header "Cookie: express.sid=$TOKEN" \
		"https://app.wercker.com/api/v3/workflows?applicationId=$1&limit=$2&skip=$3" |
	python -c """
import sys, json

for item in json.load(sys.stdin):
	print(item['id'])
"""
}

function log() {
	IDs=($(curl -sS --request GET --header "Cookie: express.sid=$TOKEN" \
		https://app.wercker.com/api/v3/workflows/$1 |
	python -c """
import sys, json

status = ''
for item in json.load(sys.stdin)['items']:
	print(item['data']['runId'])
"""))

	for ID in "${IDs[@]}"; do
		STEPs=($(curl -sS --request GET --header "Cookie: express.sid=$TOKEN" \
				https://app.wercker.com/api/v3/runs/$ID/steps |
			python -c """
import sys, json

logs = {}
i = 0

for item in json.load(sys.stdin):
	if item['status'] == 'finished':
		logs[item['order']] = item['logUrl']
	else:
		break

while i in logs:
	print(logs[i])
	i += 1
"""))

		for STEP in ${STEPs[@]}; do
			curl -sS --request GET --header "Cookie: express.sid=$TOKEN" $STEP
		done
	done
}

function status() {
	curl -sS --request GET --header "Cookie: express.sid=$TOKEN" \
		https://app.wercker.com/api/v3/workflows/$1 |
	python -c """
import sys, json

status = ''
for item in json.load(sys.stdin)['items']:
	if len(status) > 0:
		break
	elif item['status'] != 'finished':
		status = 'running'
	elif item['result'] == 'failed':
		status = 'failed'
else:
	status = 'passed'

print(status)
"""
}

function restart() {
	IDs=($(curl -sS --request GET --header "Cookie: express.sid=$TOKEN" \
		https://app.wercker.com/api/v3/workflows/$1 |
	python -c """
import sys, json

status = ''
for item in json.load(sys.stdin)['items']:
	print(item['data']['runId'])
"""))

	for ID in "${IDs[@]}"; do
		PIPELINE=$(curl -sS --request GET --header "Cookie: express.sid=$TOKEN" \
				https://app.wercker.com/api/v3/runs/$ID |
		       python -c "import sys, json; print(json.load(sys.stdin)['pipeline']['id'])")

		if [[ $# -gt 2 ]]; then
			curl -sS --request POST  										\
				 --header 'Content-Type: application/json' 							\
				 --header "Cookie: express.sid=$TOKEN" 								\
				 --data-binary "{\"pipelineId\": \"$PIPELINE\", \"branch\": \"$2\", \"commitHash\": \"$3\"}" 	\
					'https://app.wercker.com/api/v3/runs' >& /dev/null
		elif [[ $# -gt 2 ]]; then
			curl -sS --request POST  							\
				 --header 'Content-Type: application/json' 				\
				 --header "Cookie: express.sid=$TOKEN" 					\
				 --data-binary "{\"pipelineId\": \"$PIPELINE\", \"branch\": \"$2\"}" 	\
					'https://app.wercker.com/api/v3/runs' >& /dev/null
		else
			curl -sS --request POST  					\
				 --header 'Content-Type: application/json' 		\
				 --header "Cookie: express.sid=$TOKEN" 			\
				 --data-binary "{\"pipelineId\": \"$PIPELINE\"}" 	\
					'https://app.wercker.com/api/v3/runs' >& /dev/null
		fi


		while [ 1 ]; do
			sleep 3
			LOGs=($(curl -g -sS --request GET --header "Cookie: express.sid=$TOKEN" \
					"https://app.wercker.com/api/v3/runs/$ID/steps" |
				python -c """
import sys, json, os

def touch(fname):
	if os.path.exists(fname):
		os.utime(fname, None)
	else:
		open(fname, 'a').close()

results = {}

for item in json.load(sys.stdin):
	if os.path.exists('%s.fin' % item['id']):
		continue
	elif item['status'] == 'finished':
		touch('%s.don' % item['id'])

	if item['result'] == 'failed':
		touch('%s.don' % item['id'])

		if not os.path.exists('%s.fin' % item['id']):
			results[item['order']] = item['id']
		break
	else:
		results[item['order']] = item['id']

printed = False
idx = 0

while True:
	if not idx in results:
		if printed is True:
			break
	else:
		print(str(results[idx]))
		printed = True
	idx += 1
"""
))

			for LOG in "${LOGs[@]}"; do
				TMP="$LOG.tmp"
				FIN="$LOG.fin"

				if [ -f $LOG.don ]; then	
					curl -g -sS --request GET --header "Cookie: express.sid=$TOKEN" 	\
						"https://app.wercker.com/api/v3/runsteps/$LOG/log" > "$FIN"

					if [ -f $LOG ]; then
						python -c """
import sys

with open('$FIN', 'r') as fd:
	beg = $(wc -c $LOG | awk '{ print $1 }')

	fd.seek(beg - 1 if beg > 0 else 0)
	sys.stdout.write(fd.read()[:-1])
	sys.stdout.flush()
"""
					else
						cat "$FIN"
					fi
					rm -fr "$LOG"
					rm -fr "$LOG.don"

				else
					curl -g -sS --request GET --header "Cookie: express.sid=$TOKEN" 	\
						"https://app.wercker.com/api/v3/runsteps/$LOG/log" > "$TMP"

					if [ -f $LOG ]; then
						python -c """
import sys

with open('$TMP', 'r') as fd:
	beg = $(wc -c $LOG | awk '{ print $1 }')

	fd.seek(beg - 1 if beg > 0 else 0)
	sys.stdout.write(fd.read()[:-1])
	sys.stdout.flush()
"""
					else
						cat "$TMP"
					fi
					mv $TMP $LOG
	
				fi

				if [ -f $LOG ]; then
					if cat $LOG | grep 'Storing artifacts complete' >& /dev/null; then
						mv $LOG $LOG.fin
						touch finish
					fi
				elif [ -f $FIN ]; then
					if cat $FIN | grep 'Storing artifacts complete' >& /dev/null; then
						mv $LOG $LOG.fin
						touch finish
					fi
				fi
			done

			if [ -f finish ]; then
				break
			fi
		done

		rm -fr *.fin
		rm -fr finish
	done
}

function application() {
	curl -sS --request GET --header "Cookie: express.sid=$TOKEN" \
	 	"https://app.wercker.com/api/v3/applications/hung0913208/build" |
	python -c "import sys, json; print(json.load(sys.stdin)['id'])"
}

if [ $1 = 'restart' ] || [ $1 = 'status' ] || [ $1 = 'log' ]; then
	TASK=$1
	shift
	
	if ! options=$(getopt -l token,repo: -- "$@"); then
		error "Can' parse $0 $TASK $@"
	fi

	while [ $# -gt 0 ]; do
		case $1 in
			--repo)		REPO="$2"; shift;;
			--token)	TOKEN="$2"; shift;;
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

	WORKFLOW=$(workflows $(application $REPO) 1 0)

	if [ $TASK = 'status' ]; then
		status $WORKFLOW
	elif [ $TASK = 'log' ]; then
		log $WORKFLOW
	elif [ $TASK = 'restart' ]; then
		restart $WORKFLOW
	fi
elif [ $1 = 'env' ]; then
	PROTECTED="false"
	CMD=$2
	shift
	shift

	if ! options=$(getopt -l token,sid,name,value,repo,private: -- "$@"); then
		error "Can' parse $0 $TASK $@"
	fi

	while [ $# -gt 0 ]; do
		case $1 in
			--repo)		REPO="$2"; shift;;
			--token)	TOKEN="$2"; shift;;
			--name)		KEY="$2"; shift;;
			--value) 	VALUE="$2"; shift;;
			--private) 	PROTECTED="$2"; shift;;
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

	APP=$(application $REPO)

	if [ $CMD = 'add' ]; then
		curl -sS --request POST --header "Cookie: express.sid=$TOKEN" 											\
 			 --data-binary "{\"scope\":\"application\",\"target\":\"$APP\",\"key\":\"$KEY\",\"value\":\"$VALUE\",\"protected\":$PROTECTED}" 	\
				https://app.wercker.com/api/v3/envvars |
		python -c "import sys, json; sys.exit(0 if 'id' in json.load(sys.stdin) else -1)"
		exit $?
	elif [ $CMD = 'del' ]; then
		VID=$(curl -sS --request GET --header "Cookie: express.sid=$TOKEN" 	\
			"https://app.wercker.com/api/v3/envvars?scope=application&target=$APP" |
		python -c """
import sys, json

for item in json.load(sys.stdin)['results']:
	if item['key'] == '$KEY':
		print(item['id'])
		break
		""")

		if [[ ${#VID} -gt 0 ]]; then
			curl -sS --request DELETE --header "Cookie: express.sid=$TOKEN" 	\
				https://app.wercker.com/api/v3/envvars/$VID |
			python -c "import sys, json; sys.exit(-1 if 'error' in json.load(sys.stdin) else 0)"
			exit $?
		fi
	fi
fi
