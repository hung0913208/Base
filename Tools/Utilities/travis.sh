#!/bin/bash

WEBSOCKET="$(dirname $0)/../Websocket/Websocket.sh"
BUILD=$(python -c "print('$2'.split('.')[0])")
JOB=$(python -c "print('$2'.split('.')[-1])")

if [ -f $HOME/environment.sh ]; then
	source $HOME/environment.sh
fi

warning(){
	if [ $# -eq 2 ]; then
		echo "[ WARNING ]: $1 line ${SCRIPT}:$2"
	else
		echo "[ WARNING ]: $1"
	fi
}

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

	if ! curl -sS --request GET 				\
			--header "Authorization: token $TOKEN" 	\
			--header "Travis-API-Version: 3" 	\
			"https://api.travis-ci.org/repo/$REPO" |
		python -c """
from pprint import pprint
from json import load
from sys import stdin, exit

try:
	print(load(stdin)['id'])
except Exception:
	exit(-1)	
	"""; then
		exit -1
	fi

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
try:
	print(json.load(sys.stdin)['@pagination']['limit'])
except Exception:
	sys.exit(-1)
""")

			if [ $? != 0 ]; then
				exit -1
			fi
		fi

		if [[ ${#LAST} -eq 0 ]]; then
			LAST=$(echo $RESP | python -c """
import json, sys
try:
	print(json.load(sys.stdin)['builds'][0]['number'])
except Exception:
	sys.exit(-1)
""")
			if [ $? != 0 ]; then
				exit -1
			fi
		fi

		if [[ ${#IDX} -eq 0 ]]; then
			IDX=$LAST
		fi

		echo $RESP | python -c """
import json, sys
try:
	for item in json.load(sys.stdin)['builds']:
		print(item['id'])
except Exception:
	sys.exit(-1)
		"""

		if [ $? != 0 ]; then
			exit -1
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

function jobs() {
	ID=$1
	RESP=$(curl -sS --request GET 				\
			--header "Authorization: token $TOKEN" 	\
			--header "Travis-API-Version: 3" 	\
			https://api.travis-ci.org/build/$ID/jobs)

	echo $RESP | python -c """
import json, sys

try:
	for item in json.load(sys.stdin)['jobs']:
		print(item['id'])
except Exception:
	sys.exit(-1)
"""
	if [ $? != 0 ]; then
		exit -1
	fi
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

	if [ $? != 0 ]; then
		exit -1
	fi
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

try:
	print(json.load(sys.stdin)['@pagination']['limit'])
except Exception:
	sys.exit(-1)
""")

			if [ $? != 0 ]; then
				exit -1
			fi
		fi

		if [[ ${#LAST} -eq 0 ]]; then
			LAST=$(echo $RESP | python -c """
import json, sys

try:
	print(json.load(sys.stdin)['builds'][0]['number'])
except Exception:
	sys.exit(-1)
""")
			if [ $? != 0 ]; then
				exit -1
			fi
		fi

		if [[ ${#IDX} -eq 0 ]]; then
			IDX=$LAST
		fi

		IDX=$((IDX-LIMIT))
		OFFSET=$((OFFSET+LIMIT))

		if [[ $1 -gt $IDX ]]; then
			echo $RESP | python -c """
import json, sys

try:
	for idx, item in enumerate(json.load(sys.stdin)['builds']):
		if item['number'] == '$1':
			print(item['id'])
			break
except Exception:
	sys.exit(-1)
			"""

			if [ $? != 0 ]; then
				exit -1
			fi
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
	if [ $# != 2 ]; then
		echo 'unknown'
	else
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
	else:
		print('unknown')
except Exception:
	if raw.get('@type') == 'error':
		print('unknown')
	else:
		print(raw)
"""
		if [ $? != 0 ]; then
			exit -1
		fi
	fi
}

function job() {
	if [ $# != 2 ]; then
		exit -1
	fi

	ID=$1
	RESP=$(curl -sS --request GET 					\
			--header "Authorization: token $TOKEN" 		\
			--header "Travis-API-Version: 3" 		\
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
	if [ $? != 0 ]; then
		exit -1
	fi
}

function log() {
	if [ $# != 2 ]; then
		exit -1
	fi

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

try:
	raw = json.load(sys.stdin)

	if raw.get('@type') == 'error':
		sys.exit(-1)
	else:
		print(unquote_plus(raw.get('content') or ''))
except Exception:
	sys.exit(-1)
"""

	if [ $? != 0 ]; then
		exit -1
	fi
}

function delete() {
	if [ $# != 2 ]; then
		exit -1
	fi

	BUILD=$1
	JOB=$2
	curl -sS --request DELETE                           \
		 --header "Authorization: token $TOKEN"     \
		 --header "Travis-API-Version: 3"           \
		https://api.travis-ci.org/job/$(job $BUILD $JOB)/log >& /dev/null
}

function console() {
	USER=$(curl -sS --request GET 					\
			        --header "Authorization: token $TOKEN" 	\
        			--header "Travis-API-Version: 3" 	\
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
pending = {}
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
expected = 0

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

        if num == expected:
            sys.stdout.write(log.encode('ascii', 'ignore').decode('ascii'))
            sys.stdout.flush()
            expected += 1
            num += 1

            while True:
                if num != expected or not num in pending:
                    break

                sys.stdout.write(pending[num].encode('ascii', 'ignore').decode('ascii'))
                sys.stdout.flush()

                del pending[num]
                expected += 1
                num += 1
        else:
            pending[num] = log
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
try:
	for line in sys.stdin.readlines():
		if 'environment' in line:
			content = line.split('content=\"')[1].split('\" />')[0]
	else:
		print(unquote_plus(content))
except Exception:
	sys.exit(-1)
""")

if [ $? != 0 ]; then
	exit -1
fi

PUSHER=$(echo $ENV | python -c """
import json, sys
try:
	print(json.load(sys.stdin)['pusher']['key'])
except Exception:
	sys.exit(-1)
""")

if [ $? != 0 ]; then
	exit -1
fi

if [ $1 = 'restart' ] || [ $1 = 'status' ] || [ $1 = 'log' ] || [ $1 = 'console' ] || [ $1 = 'delete' ]; then
	TASK=$1

	if [ $1 = 'log' ] || [ $1 = 'status' ]; then
		JOB=1
	fi
	shift
	
	if ! options=$(getopt -l token,patch,job,repo,script: -- "$@"); then
		error "Can' parse $0 $TASK $@"
	fi

	while [ $# -gt 0 ]; do
		case $1 in
			--expected)     STEP="$2"; shift;;
			--job)		JOB="$2"; shift;;
			--repo)		REPO="$2"; shift;;
			--patch)	BUILD="$2"; shift;;
			--token)	TOKEN="$2" ; shift;;
			--script)
					if [ $TASK = 'restart' ]; then
						SCRIPT=$2
						shift
					else
						error "unrecognized option $1"
					fi;;
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
	PATCH=$BUILD
	BUILD=$(build $BUILD)

	if [ $TASK = 'log' ] || [ $TASK = 'delete' ]; then
		STATUS=$(status $BUILD $JOB)
       	
        	if [[ ${#STATUS} -eq 0 ]]; then
			exit 0
		elif [  $STATUS != 'started' ] && [ $TASK = 'log' ]; then
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
try:
	print(unquote_plus(json.load(sys.stdin)['content']))
except Exception:
	sys.exit(-1)
			"""

			if [ $? != 0 ]; then
				exit -1
			fi
		fi

		if [ $TASK = 'delete' ]; then
			delete $BUILD $JOB
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
			mkdir -p /tmp/$REPO

			if [ ! -f /tmp/$REPO/$ID ]; then
				cat > /tmp/$REPO/$ID << EOF
#!/bin/bash

while [ 1 ]; do
	STATUS=\$($0 status --repo $REPO --job $ID --token $TOKEN --patch $PATCH)

	if [ "\$STATUS" = 'started' ]; then
		rm -fr /tmp/$REPO/$ID
		bash $SCRIPT
		break
	else
		sleep 1
	fi
done
EOF

				chmod +x /tmp/$REPO/$ID
			else
				error "your system run faster than expected"
			fi

			atd
			at -f /tmp/$REPO/$ID now

			console 'restarted' $JOB $PUSHER
		elif [ $(status $BUILD $ID) = 'started' ]; then
			console 'started' $JOB $PUSHER $BUILD $ID
		fi

		if [ $(status $BUILD $ID) = 'failed' ]; then
			exit -1
		elif [ $(status $BUILD $ID) = 'errored' ]; then
			exit -2
		elif [ $(status $BUILD $ID) = 'passed' ]; then
			exit 0
		else
			echo "[ WARNING ]: it seem we are in the middle of unxpected state $(status $BUILD $ID)"
			exit -2
		fi
	fi
elif [ $1 = 'env' ]; then
	shift

	if [ $1 = 'add' ]; then
		shift

		BRANCH='null'
		PUBLIC='true'

		if ! options=$(getopt -l repo,token,name,value,branch,public: -- "$@"); then
			error "Can' parse $0 env add $@"
		fi

		while [ $# -gt 0 ]; do
			case $1 in
				--repo)		REPO="$2"; shift;;
				--name) 	NAME="$2"; shift;;
				--token)	TOKEN="$2"; shift;;
				--value) 	VALUE="$2"; shift;;
				--branch) 	BRANCH="\"$2\""; shift;;
				--public) 	PUBLIC="true";;
				--private)	PUBLIC="false";;
				(--) 		shift; break;;
				(-*) 		error "unrecognized option $1";;
				(*) 		break;;
			esac
			shift
		done

		REPO=$(repository $REPO)
		curl -sS --request POST 						\
                        	 --header "Authorization: token $TOKEN"     		\
				 --header "Accept: application/json; version=2" 	\
				 --data-binary "{\"env_var\":{\"name\":\"$NAME\",\"value\":\"$VALUE\",\"public\":$PUBLIC,\"branch\":$BRANCH,\"repository_id\":\"$REPO\"}}" \
			https://api.travis-ci.org/settings/env_vars?repository_id=$REPO |
		python -c """
import json, sys

resp = json.load(sys.stdin)

if not 'env_var' in resp:
	sys.exit(-1)
elif not 'id' in resp['env_var']:
	sys.exit(-1)
"""
		if [ $? != 0 ]; then
			exit -1
		fi
	elif [ $1 = 'del' ]; then
		shift

		if ! options=$(getopt -l repo,token,name: -- "$@"); then
			error "Can' parse $0 env set $@"
		fi

		while [ $# -gt 0 ]; do
			case $1 in
				--repo)		REPO="$2"; shift;;
				--name) 	NAME="$2"; shift;;
				--token)	TOKEN="$2"; shift;;
				(--) 		shift; break;;
				(-*) 		error "unrecognized option $1";;
				(*) 		break;;
			esac
			shift
		done

		REPO=$(repository $REPO)
		ID=$(curl -sS --request GET 						\
                              --header "Authorization: token $TOKEN"     		\
			https://api.travis-ci.org/settings/env_vars?repository_id=$REPO |
		python -c """
import json, sys
try:
	env = json.load(sys.stdin)
	for item in env['env_vars']:
		if item['name'] == '$NAME':
			print(item['id'])
			break
except Exception:
	sys.exit(-1)
		""")

		if [ $? != 0 ]; then
			exit -1
		fi
		curl -sS --request DELETE 				\
                         --header "Authorization: token $TOKEN"     	\
		"https://api.travis-ci.org/settings/env_vars/$ID?repository_id=$REPO" |
		python -c """
import json, sys
try:
	resp = json.load(sys.stdin)

	if not 'env_var' in resp:
		sys.exit(-1)
	elif not 'id' in resp['env_var']:
		sys.exit(-1)
except Exception:
	sys.exit(-1)
"""

		exit $?
	elif [ $1 = 'exist' ]; then
		shift

		if ! options=$(getopt -l repo,token,name,token: -- "$@"); then
			error "Can' parse $0 env set $@"
		fi

		while [ $# -gt 0 ]; do
			case $1 in
				--token)	TOKEN="$2"; shift;;
				--name)		NAME="$2"; shift;;
				--repo)		REPO="$2"; shift;;
				(--) 		shift; break;;
				(-*) 		error "unrecognized option $1";;
				(*) 		break;;
			esac
			shift
		done

		REPO=$(repository $REPO)

		curl -sS --request GET 							\
                              --header "Authorization: token $TOKEN"     		\
			https://api.travis-ci.org/settings/env_vars?repository_id=$REPO |
		python -c """
import json, sys

try:
	env = json.load(sys.stdin)
	for item in env['env_vars']:
		if '$NAME' == item['name']:
			sys.exit(0)
	else:
		sys.exit(-1)
except Exception:
	sys.exit(-1)
		"""
		exit $?
	elif [ $1 = 'list' ]; then
		shift

		if ! options=$(getopt -l repo,token,script: -- "$@"); then
			error "Can' parse $0 env set $@"
		fi

		while [ $# -gt 0 ]; do
			case $1 in
				--repo)		REPO="$2"; shift;;
				--script) 	SCRIPT="$2"; shift;;
				--token)	TOKEN="$2"; shift;;
				(--) 		shift; break;;
				(-*) 		error "unrecognized option $1";;
				(*) 		break;;
			esac
			shift
		done

		REPO=$(repository $REPO)
		curl -sS --request GET 							\
                              --header "Authorization: token $TOKEN"     		\
			https://api.travis-ci.org/settings/env_vars?repository_id=$REPO |
		python -c """
import json, sys, subprocess
try:
	env = json.load(sys.stdin)
	for item in env['env_vars']:
		cmds = '$SCRIPT'.split(' ')

		cmds.append(str(item['name']))
		cmds.append(str(item['value']))
		subprocess.call(cmds)
except Exception:
	sys.exit(-1)
		"""
		exit $?
	fi
fi
