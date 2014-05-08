#!/bin/sh
# Type './build.sh' to make Android native libraries.
# Type './build.sh -B' to rebuild the native libraries.
# Type `./build.sh APP_ABI=x86` to build for the x86 Emulator.
#
cd TouchVGCore/jni
if [ "$1"x = "-swig"x ] ; then
    ndk-build $2
else
    ndk-build $1 $2
fi
cd ../..
