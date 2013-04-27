export GYP_GENERATORS='xcode'
# export GYP_DEFINES="clang=1"
# export CC=clang
# export CXX=clang++

./Tools/build/gyp/gyp --generator-output=xcode --depth=0 --toplevel-dir . ./Source/daedalus.gyp
