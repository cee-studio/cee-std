#!/bin/bash
set -e
set -o pipefail

mypath=$(dirname $(readlink -f $0))
url="https://raw.githubusercontent.com/cee-studio/cee-std/master"
url="file:///${HOME}/workspace/cee-std"

curl $url/scripts/get-cee-std.sh -o ${mypath}/get-cee-std.sh
chmod +x ${mypath}/get-cee-std.sh

cee_list="
cee.h
cee.c"

json_list="
cee-json.h
cee-json.c"

sqlite3_list="
join.c
cee-sqlite3.h
cee-sqlite3.c"

mkdir -p $mypath/../cee-std
pushd $mypath/../cee-std
for i in $cee_list; do
    echo "getting $i"
    echo "$url/release/$i"
    curl $url/release/$i -o $i
done

for i in $json_list; do
    echo "getting $i"
    echo "$url/cee-json/release/$i"
    curl $url/cee-json/release/$i -o $i
done

for i in $sqlite3_list; do
    echo "getting $i"
    echo "$url/sqlite3/$i"
    curl $url/sqlite3/$i -o $i
done
popd
