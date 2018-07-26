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
elif uname | grep Linux >/dev/null; then
  cd "$ROOT/spssio/lin64"

  ln -s "$PWD/libspssdio.so.1" "$PWD/libspssdio.so"
  ln -s "$PWD/libicudata.so.51.2" "$PWD/libicudata.so.51"
  ln -s "$PWD/libicui18n.so.51.2" "$PWD/libicui18n.so.51"
  ln -s "$PWD/libicuuc.so.51.2" "$PWD/libicuuc.so.51"
fi

