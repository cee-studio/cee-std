#!/bin/bash
set -e
set -o pipefail

mypath=$(dirname $(readlink -f $0))
url="https://raw.githubusercontent.com/cee-studio/cee-std/master"

wget $url/scripts/get-cee-std.sh -O ${mypath}/get-cee-std.sh
chmod +x ${mypath}/get-cee-std.sh

list="
cee-json/cee-json.h
cee-json/cee-json.c
cee.h
cee.c"

mkdir -p $mypath/../cee-std
pushd $mypath/../cee-std
for i in $list; do
    echo "getting $i"
    echo "$url/release/$i"
    wget $url/release/$i -O $i
done
popd
