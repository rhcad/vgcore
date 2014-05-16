# TouchVGCore

## Overview

TouchVGCore contains the following cross-platform modules using C++ for 2D vector drawing frameworks:

- geom: Math and geometry module.
- graph: 2D Graphics interface module using geom module.
- shape: 2D Shape module using geom and graph modules.
- shapedoc: Shape document module using above modules.
- jsonstorage: A storage adapter using [rapidjson](https://github.com/Kanma/rapidjson)(a fast JSON parser/generator).
- cmdbase: Base classes for deriving more drawing commands.
- cmdbasic: Commands to draw basic shapes.
- cmdmgr: Command manager module.
- view: Interactive drawing kernel module.
- export: SVG exportor module.
- record: Undo/redo and shape record module.

![modules](/doc/images/modules.png)

This is an open source LGPL 2.1 licensed project that is in active development. Contributors and sponsors are welcome.

## Build

### Build for **iOS** platform on Mac OS X.

TouchVGCore is available on [CocoaPods](http://cocoapods.org). Just add the following to your project Podfile:

```ruby
pod 'TouchVGCore', '~> 0.29'
```

Alternatively, you can add the project to your workspace and build as one of the following methods:

- Open `ios/TouchVGCore/TouchVGCore.xcodeproj` in Xcode, then build the library project.

- Or cd the 'ios' folder of this project and type `./build.sh` to build `ios/output/libTouchVGCore.a`.
   - Type `./build.sh -arch arm64` to make iOS libraries for iOS 64-bit. Type `./build.sh clean` to remove object files.

### Build for **Android** platform on Mac, Linux or Windows.

- Cd the 'android' folder of this project and type `./build.sh` to build with ndk-build.
  - MSYS is recommend on Windows.
  - The library `libTouchVGCore.a` will be outputed to `android/TouchVGCore/obj/local/armeabi`.
  - Type `./build.sh -B` to rebuild the native libraries.

- Type `./build.sh APP_ABI=x86` to build for the x86 Emulator. The library will be outputed to `android/TouchVGCore/obj/local/x86`.

### Build for **Windows** platform with Visual Studio.

- Open `win\vc2010.sln` in Visual Studio 2010, then build the TouchVGCore library project (Win32 VC++ static library).
   
### Build for more platforms and targets.

- Cd the 'core' folder of this project and type `make` or `make all install` to generate libraries on Mac, Linux or Windows.

- Type `make java`, `make python` or `make csharp` to generate libraries for another language applications using Java, Python or C#.

- Type `make clean java.clean python.clean` to remove the program object files.
