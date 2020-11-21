cmake_minimum_required(VERSION 3.14.0)
project(jcc)
set(CMAKE_CXX_STANDARD 17)
set(SHARED_CC_OPTIONS -fno-rtti -Wall -Wpedantic -Wextra -Werror)
set(DEBUG_CC_OPTIONS -g -O0 ${SHARED_CC_OPTIONS} -fno-omit-frame-pointer)
set(RELEASE_CC_OPTIONS -O3 ${SHARED_CC_OPTIONS})
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == Projects ===
set(JCC_LIB libjcc)
set(COMPILER jcc)
set(INTERPRETER jrepl)

# ===Tooling===
# This should be a git push hook - running it on each build is very expensive
# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=cppcoreguidelines-*,-cppcoreguidelines-pro-type-vararg,-cppcoreguidelines-pro-bounds-array-to-pointer-decay,-cppcoreguidelines-avoid-magic-numbers,-cppcoreguidelines-non-private-member-variables-in-classes")
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message("-- Found CCache - ${CCACHE_PROGRAM}")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# 3p
add_subdirectory(3p/googletest)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm_libs support core nativecodegen transformutils orcjit native)
message("-- Found LLVM libs - ${llvm_libs}")

# Sources
add_subdirectory(src)

# Unit tests
enable_testing()
add_subdirectory(test)
