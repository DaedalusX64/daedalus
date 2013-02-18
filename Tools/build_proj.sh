export GYP_GENERATORS='ninja'
export GYP_DEFINES="clang=1"
export PATH=$PATH:~/clang3.1/bin

export CC=~/clang3.1/bin/clang
export CXX=~/clang3.1/bin/clang++

./Tools/build/gyp/gyp --depth=0 --toplevel-dir . ./daedalus.gyp
