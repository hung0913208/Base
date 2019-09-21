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
	print(error)
	sys.exit(-1)
"""
}

function build() {
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

		IDX=$((IDX-LIMIT))
		OFFSET=$((OFFSET+LIMIT))

		if [[ $1 -gt $IDX ]]; then
			echo $RESP | python -c """
import json, sys

for idx, item in enumerate(json.load(sys.stdin)['builds']):
	if item['number'] == '$1':
		print(item['id'])
		break
			"""
			break
		fi

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

raw = json.load(sys.stdin)
try:
	for idx, item in enumerate(raw['jobs']):
		if idx + 1 == $2:
			print(item['state'])
			break
except Exception:
	print(raw)
"""
}

function job() {
	ID=$1
	RESP=$(curl -sS --request GET 				    \
			--header "Authorization: token $TOKEN" 	\
			--header "Travis-API-Version: 3" 	    \
			https://api.travis-ci.org/build/$ID/jobs)

	echo $RESP | python -c """
import json, sys

raw = json.load(sys.stdin)
try:
	for idx, item in enumerate(raw['jobs']):
		if idx + 1 == $2:
			print(item['id'])
			break
except Exception:
	sys.exit(-1)
"""
}

function log() {
	BUILD=$1
	JOB=$2
	curl -sS --request GET                              \
		 --header "Authorization: token $TOKEN"     \
		 --header "Travis-API-Version: 3"           \
		https://api.travis-ci.org/job/$(job $BUILD $JOB)/log |
	python -c """
import json, sys
try:
    from urllib import unquote_plus
except ImportError:
    from urllib.parse import unquote_plus
raw = json.load(sys.stdin)

print(unquote_plus(raw.get('content') or ''))
"""
}

function console() {
	USER=$(curl -sS --request GET 				            \
			        --header "Authorization: token $TOKEN" 	\
        			--header "Travis-API-Version: 3" 	    \
            https://api.travis-ci.org/user?include=owner.installation |
            python -c "import json, sys; print(json.load(sys.stdin)['id'])")
	if [ "$1" = 'started' ] || [ $1 = 'restarted' ]; then
		JOB=$2
		REPO=''
	else
		JOB=''
		REPO=$2
    	fi

	PUSHER=$3
	PROTOCOL="7"
	VERSION="5.0.0"

	CHANNELS=$(curl -sS --request GET 				\
                        --header "Authorization: token $TOKEN" 		\
                        --header "Accept: application/json; version=2"  \
                    "https://api.travis-ci.org/users/$USER" |
               python -c """
import json, sys
print(str(json.load(sys.stdin)['user']['channels']))
""")

	cat > travis.py << EOF
try:
    from urllib2 import Request, urlopen
except ImportError:
    from urllib.request import urlopen, Request
import json, sys

job = 'job-$JOB'
keep = True
channels = {}

@ws.Hook('ping')
def Pong():
    ws.Send(json.dumps({'event':'pusher:ping', 'data':{}}))

@ws.Hook('closed')
def Close():
    keep = False

def Authenticate(sock, channels):
    headers = {
        'Authorization': 'token $TOKEN',
        'Content-Type': 'application/json',
        'Accept': 'application/json; version=2'
    }
    values = {'socket_id': sock, 'channels': []}
    for channel in channels:
        if 'job' in channel:
            continue
        else:
            values['channels'].append(channel)

    try:
        data = json.dumps(values)
        url = 'https://api.travis-ci.org/pusher/auth'
        req = Request(url, data=data, headers=headers)
        resp = urlopen(req)

        return json.load(resp)
    except Exception as e:
        sys.exit(-1)

for channel in $CHANNELS:
    channels[channel] = False

stage = 'establishing'
index = 0

while keep is True:
    try:
        if stage != 'registering:2':
            raw = str(ws.Recv())

            if len(raw) == 0:
    	       continue
            else:
                resp = json.loads(raw)
    except ValueError as e:
        continue
    event = resp['event']

    if event == 'pusher:error':
        print(resp['data']['message'])
        sys.exit(-1)
    elif event.split(':')[1] == 'connection_established' or stage.split(':')[0] == 'registering':
        if stage == 'establishing':
            socket = json.loads(resp['data'])['socket_id']
            stage = 'registering:0'

            if len('$REPO') > 0:
                ws.Send(json.dumps({
                    "event": "pusher:subscribe",
                    "data": {
                        "channel": "repo-$REPO"
                    }
                }))
            elif len('$JOB') > 0:
                ws.Send(json.dumps({
                    "event": "pusher:subscribe",
                    "data": {
                        "channel":"job-$JOB"
                    }
                }))
	
            stage = 'registering:1'
            continue
        elif stage == 'registering:1':
            if event.split(':')[1] == 'subscription_succeeded':
                if resp['channel'] == 'repo-$REPO':
                    stage = 'registering:2'
                elif resp['channel'] == 'job-$JOB':
                    stage = 'registering:2'
                else:
                    stage = 'registering:0'
            else:
                stage = 'registering:0'
            continue
        elif stage == 'registering:2':
            auth = Authenticate(socket, channels)
            stage = 'registering:3'

        while True:
            if stage == 'registering:3':
                if len(channels) > index:
                    ws.Send(json.dumps({
                        "event": "pusher:subscribe",
                        "data": {
			    "auth": auth['channels'][channels.keys()[index]],
                            "channel": channels.keys()[index]
                        }
                    }))
                    stage = 'registering:4'
                else:
                    stage = 'established'
                break
            elif stage == 'registering:4':
	        if resp['channel'] == channels.keys()[index]:
                    channels[resp['channel']] = True
                    stage = 'registering:3'
                    index += 1
                else:
                    break
    elif event.split(':')[1] == 'subscription_succeeded':
        if resp['channel'] in channels:
            channels[resp['channel']] = True
    elif event == 'job:log' and resp['channel'] == job:
        import sys

        parsed = json.loads(resp['data'])
        log = parsed['_log']
        num = parsed['number']
        keep = parsed['final'] == False

	sys.stdout.write(log.encode('ascii', 'ignore').decode('ascii'))
        sys.stdout.flush()
EOF

	if [[ $# -gt 3 ]]; then
		log $4 $5
	fi

	$WEBSOCKET "ws://ws.pusherapp.com/app/$PUSHER?protocol=$PROTOCOL&client=js&version=$VERSION&flash=false" $(pwd)/travis.py
    return $?
}

ENV=$(curl -sS --request GET https://travis-ci.org/dashboard | python -c """
try:
    from urllib import unquote_plus
except ImportError:
    from urllib.parse import unquote_plus
import sys

for line in sys.stdin.readlines():
    if 'environment' in line:
        content = line.split('content=\"')[1].split('\" />')[0]
else:
    print(unquote_plus(content))
""")

PUSHER=$(echo $ENV | python -c """
import json, sys
print(json.load(sys.stdin)['pusher']['key'])
""")

if [ $1 = 'restart' ] || [ $1 = 'status' ] || [ $1 = 'log' ] || [ $1 = 'console' ]; then
	TASK=$1

	if [ $1 = 'log' ] || [ $1 = 'status' ]; then
		JOB=1
	fi
	shift
	
	if ! options=$(getopt -l token,patch,job,repo: -- "$@"); then
		error "Can' parse $BASE/Tools/Builder/build $@"
	fi

	while [ $# -gt 0 ]; do
		case $1 in
			--job)		JOB="$2"; shift;;
			--repo)		REPO="$2"; shift;;
			--patch)	BUILD="$2"; shift;;
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

	if [ $TASK = 'log' ]; then
		STATUS=$(status $BUILD $JOB)
        
        	if [[ ${#STATUS} -eq 0 ]]; then
			exit 0
		elif [  $STATUS != 'started' ]; then
                	curl -sS --request GET                              \
                        	 --header "Authorization: token $TOKEN"     \
	                         --header "Travis-API-Version: 3"           \
				https://api.travis-ci.org/job/$(job $BUILD $JOB)/log |
	                python -c """
import json, sys
try:
    from urllib import unquote_plus
except ImportError:
    from urllib.parse import unquote_plus

print(unquote_plus(json.load(sys.stdin)['content']))
			"""
		fi
	elif [ $TASK = 'status' ]; then
		status $BUILD $JOB
	elif [[ ${#JOB} -eq 0 ]]; then
		if ! restart 'build' $BUILD; then
			error "Can't restart travis"
		fi
	else
		ID=$JOB
		JOB=$(job $BUILD $JOB)
	
		if [ $TASK = 'console' ]; then
        		STATUS=$(status $BUILD $JOB)

			if [ "$STATUS" = 'started' ]; then
				console "started" $JOB $PUSHER $BUILD $ID
			else
				log $BUILD $ID
			fi
		elif restart 'job' $JOB $BUILD; then
			console 'restarted' $JOB $PUSHER
		elif [ $(status $BUILD $ID) = 'started' ]; then
			console 'started' $JOB $PUSHER $BUILD $ID
		fi
	fi
elif [ $1 = 'env' ]; then
	shift

	if [ $1 = 'set' ]; then
		shift
	fi
fi
