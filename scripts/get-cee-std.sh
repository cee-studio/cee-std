#!/bin/bash
set -e
set -o pipefail

mypath=$(dirname $(readlink -f $0))
url="https://raw.githubusercontent.com/cee-studio/cee-std/master"

wget --no-cache $url/scripts/get-cee-std.sh -O ${mypath}/get-cee-std.sh
chmod +x ${mypath}/get-cee-std.sh

cee_list="
cee.h
cee.c"

json_list="
cee-json.h
cee-json.c"

mkdir -p $mypath/../cee-std
pushd $mypath/../cee-std
for i in $cee_list; do
    echo "getting $i"
    echo "$url/release/$i"
    wget --no-cache $url/release/$i -O $i
done

for i in $json_list; do
    echo "getting $i"
    echo "$url/cee-json/release/$i"
    wget --no-cache $url/cee-json/release/$i -O $i
done
popd
