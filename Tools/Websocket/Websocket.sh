#!/bin/bash
CURRENT=$(pwd)

function on_exit() {
	cd $CURRENT
}

cd $(dirname $0) || exit -1
trap on_exit 1 2 3 15 20

# @NOTE: what would be the best way to define an interact websocket-cli?
while read LINE; do
	SCRIPT="""$SCRIPT
        $LINE
"""
done <&0

python -c """
from Source import WebSocket

if __name__ == '__main__':
    with WebSocket('$1') as ws:
        $SCRIPT
"""
exit $?

