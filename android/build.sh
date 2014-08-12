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

cd VGShape/jni
if [ "$1"x = "-swig"x ] ; then
    ndk-build $2
else
    ndk-build $1 $2
fi
cd ../..

mkdir -p output/armeabi
cp -R TouchVGCore/obj/local/armeabi/*.a output/armeabi
mkdir -p output/x86
cp -R TouchVGCore/obj/local/x86/*.a output/x86

mkdir -p output/armeabi
cp -R VGShape/obj/local/armeabi/*.a output/armeabi
mkdir -p output/x86
cp -R VGShape/obj/local/x86/*.a output/x86
