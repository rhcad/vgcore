#!/bin/sh
# Type './build.sh' to make iOS libraries.

iphoneos71=`xcodebuild -showsdks | grep -i iphoneos7.1`
iphoneos70=`xcodebuild -showsdks | grep -i iphoneos7.0`
iphoneos61=`xcodebuild -showsdks | grep -i iphoneos6.1`
iphoneos51=`xcodebuild -showsdks | grep -i iphoneos5.1`
iphoneos43=`xcodebuild -showsdks | grep -i iphoneos4.3`

if [ -n "$iphoneos71" ]; then
    xcodebuild -project TouchVGCore/TouchVGCore.xcodeproj -sdk iphoneos7.1 -configuration Release
else
if [ -n "$iphoneos70" ]; then
    xcodebuild -project TouchVGCore/TouchVGCore.xcodeproj -sdk iphoneos7.0 -configuration Release
else
if [ -n "$iphoneos61" ]; then
    xcodebuild -project TouchVGCore/TouchVGCore.xcodeproj -sdk iphoneos6.1 -configuration Release -arch armv7
else
if [ -n "$iphoneos51" ]; then
    xcodebuild -project TouchVGCore/TouchVGCore.xcodeproj -sdk iphoneos5.1 -configuration Release
else
if [ -n "$iphoneos43" ]; then
    xcodebuild -project TouchVGCore/TouchVGCore.xcodeproj -sdk iphoneos4.3 -configuration Release
fi
fi
fi
fi
fi

mkdir -p output/TouchVG
cp -R TouchVGCore/build/Release-universal/libTouchVGCore.a output
cp -R TouchVGCore/build/Release-universal/include/TouchVGCore/*.h output/TouchVG