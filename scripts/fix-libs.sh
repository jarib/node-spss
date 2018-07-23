#!/bin/bash -ex

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )
ROOT=$(dirname $DIR)

if uname | grep Darwin >/dev/null; then
  cd "$ROOT/spssio/macos"

  for l in *.dylib; do
    echo $l

    for fix in $(otool -L $l | grep -Eo '@executable_path/../lib/\S+' | grep -v $l); do
      echo "fixing $fix"
      install_name_tool -change "$fix" "@rpath/$(basename $fix)" $l
    done
  done

  install_name_tool -change "/libzlib1211spss.dylib" "@rpath/libzlib1211spss.dylib" libspssdio.dylib

  otool -L libspssdio.dylib
fi

