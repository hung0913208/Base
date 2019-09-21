#!/bin/bash
CURRENT=$(pwd)
SOURCE=$2

function on_exit() {
	rm -fr cli.py
	rm -fr $SOURCE
	cd $CURRENT
}

cd $(dirname $0) || exit -1
trap on_exit 1 2 3 15 20

cat > $(pwd)/cli.py << EOF
from Source import WebSocket

if __name__ == '__main__':
    with WebSocket('$1') as ws:
EOF

python -c """
with open('$(pwd)/cli.py', 'a') as dst:
    with open('$2', 'r') as src:
        for line in src.readlines():
            dst.write('        %s' % line)
"""

python cli.py
CODE=$?

rm -fr cli.py
rm -fr $2
exit $CODE

