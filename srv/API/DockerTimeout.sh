#!/bin/bash

# example starting command:
# 20s -u mysql -e 'NODE_PATH=/usr/local/lib/node_modules' -i -t
# -v  "/home/haolanliu/compilebox/API/temp/ea2268797c9d42f709bd":/usercode virtual_machine /usercode/script.sh python file.py

set -e

to=$1
shift

echo "arg list:"
echo "$@"

cont=$(docker run --rm -d "$@")
code=$(timeout "$to" docker wait "$cont" || true)  # to is the first args

docker kill $cont &> /dev/null
echo -n 'status: '
if [ -z "$code" ]; then
    echo timeout
else
    echo exited: $code
fi

echo output:
# pipe to sed simply for pretty nice indentation
docker logs $cont | sed 's/^/\t/'

docker rm $cont &> /dev/null
