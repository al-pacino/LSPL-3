[![Build Status](https://travis-ci.org/al-pacino/LSPL-3.svg?branch=master)](https://travis-ci.org/al-pacino/LSPL-3)

## Supported Platforms

Windows 7, 8.1, 10, Mac OS X, Linux

Other UNIX-like operating systems may work too out of the box.

## Building lspl3 on Windows using Visual Studio 2015 (or later):

- Clone this repository
- Open [lspl3.sln](lspl3.sln) solution
- Build (F7)

## Building lspl3 using CMake

- Download and install last version of [CMake](http://www.cmake.org/download/)
- Clone this repository
- Build using CMake:
```sh
# Create `build` folder
mkdir build; cd build

# You can use all cmake generators available on your platform
cmake -G "Unix Makefiles" ../

# Build executable
make
```

## Using

Run program `lspl3`:
```sh
./lspl3 ../lspl3config.json ../tests/Patterns.txt ../tests/2001_A_Space_Odyssey.json ""
```
