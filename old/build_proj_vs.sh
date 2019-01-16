export GYP_GENERATORS='msvs'
export GYP_MSVS_VERSION='2012'

./Tools/build/gyp/gyp --generator-output=w32 --depth=0 --toplevel-dir . ./Source/daedalus.gyp
