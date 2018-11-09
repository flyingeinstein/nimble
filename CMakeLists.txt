cmake_minimum_required(VERSION 3.5)

project(Nimble)

include(CMakeChecks.txt)

set(SOURCE_FILES Rest.h Rest.cpp RestTests.cpp pool.h pool.cc binbag.h binbag.cc)

add_executable(rest-tests ${SOURCE_FILES})
set_property(TARGET rest-tests PROPERTY CXX_STANDARD 11)

