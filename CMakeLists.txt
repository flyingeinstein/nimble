cmake_minimum_required(VERSION 3.5)

project(Nimble)

# check for includes
include(CheckIncludeFile)
check_include_file(string.h HAS_STRING_H)
check_include_file(strings.h HAS_STRINGS_H)

# check symbols for existance on platform
include(CheckSymbolExists)
check_symbol_exists(strndup string.h HAS_STRNDUP)
check_symbol_exists(stpcpy  string.h HAS_STPCPY)
check_symbol_exists(stpncpy string.h HAS_STPNCPY)

set(SOURCE_FILES Rest.h Rest.cpp RestTests.cpp pool.h pool.cc binbag.h binbag.cc)

add_executable(rest-tests ${SOURCE_FILES})
set_property(TARGET rest-tests PROPERTY CXX_STANDARD 11)

