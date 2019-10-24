# Recruitment test - mp3 converter:

Write a C/C++ commandline application that encodes a set of WAV files to MP3    

Requirements:

1. application is called with pathname as argument, e.g. `<applicationname> F:\MyWavCollection` all WAV-files contained directly in that folder are to be encoded to MP3
2. use all available CPU cores for the encoding process in an efficient way by utilizing multi-threading
3. statically link to lame encoder library
4. application should be compilable and runnable on Windows and Linux
5. the resulting MP3 files are to be placed within the same directory as the source WAV files, the filename extension should be changed appropriately to .MP3
6. non-WAV files in the given folder shall be ignored
7. multithreading shall be implemented in a portable way, for example using POSIX pthreads.
8. frameworks such as Boost or Qt shall not be used
9. the LAME encoder should be used with reasonable standard settings (e.g. quality based encoding with quality level "good")

## Highlights
* Aquires and compiles lame static lib via cmake from the internet, for both Win and Linux.
* Reads Wav file chunks into POD structs, and provides safe operators for comparison in these structs.
* Templated mp3 encoding process, based on raw wav file format, uses `constexpr if` where possible.
* MP3 encoding class asserts for usage with correct type_traits, throws human-readable compile error if used with unsupported type.
* Delegates tasks using std::async.
* Prevents task thrashing by submitting a limited number of tasks, based on std::thread::hardware_concurreny.

## Build requirements
A C++ compiler with C++17 standard support and c++stdlib with filesystem.
This means: GCC >= 8.0, MSVC > v141.
Machine must have internet access to retrieve lame and build requirements.

## How to build
Use the normal cmake workflow:
```
mkdir build
cd build
cmake ..
cmake --build .
```

Tested toolchains on Win:
* cmake -G "Visual Studio 16 2019" -A x64
* cmake -G "Visual Studio 16 2019" -A Win32
* cmake -G "NMake Makefiles"

In Unix / MSYS2 with MinGW64:
* cmake -G "Unix Makefiles"
