export PATH=$PATH:~/dev/depot_tools
export GYP_DEFINES=clang=1
ninja -C $1
