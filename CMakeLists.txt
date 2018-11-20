cmake_minimum_required(VERSION 3.5)

project(Nimble)

include(CMakeChecks.txt)

# package up the Nimble files into a static library
set(SOURCE_FILES Rest.h Rest.cpp binbag.h binbag.cpp)
add_library(nimble STATIC ${SOURCE_FILES})
set_property(TARGET nimble PROPERTY CXX_STANDARD 11)


add_subdirectory(tests)
