#!/bin/sh
# Type './build.sh' to make iOS libraries.
# Type './build.sh -arch arm64' to make iOS libraries for iOS 64-bit.
# Type './build.sh clean' to remove object files.

xcodebuild -project TouchVGCore/TouchVGCore.xcodeproj $1 $2 -configuration Release -target TouchVGCore

mkdir -p output/TouchVGCore
cp -R TouchVGCore/build/Release-universal/*.a output
cp -R TouchVGCore/build/Release-universal/include/TouchVGCore output
#cp -R TouchVGCore/build/Release-universal/include/VGShape output
