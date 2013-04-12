export GYP_GENERATORS='ninja'
export GYP_DEFINES="clang=1"

export CC=clang
export CXX=clang++

./Tools/build/gyp/gyp --depth=0 --toplevel-dir . ./Projects/OSX/daedalus.gyp
