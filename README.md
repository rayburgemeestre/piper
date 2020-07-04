     PIP    E       R
    [PIPelinE manageR]

## Building

    make         # build
    make debug   # build debug version
    make format  # format source code with clang-format

## Running

    ./build/example

## Usage in projects

First add as submodule:

    git submodule add https://github.com/rayburgemeestre/piper libs/piper

Then add path in your `CMakeLists.txt`

    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libs/piper/")

... TODO ...

Use in source code:

    #include "piper.h"

... TODO ...
