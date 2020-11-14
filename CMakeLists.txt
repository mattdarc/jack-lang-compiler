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

# System llvm
find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(LLVM_LIBS support core nativecodegen transformutils orcjit native)
message("-- Found LLVM libs - ${LLVM_LIBS}")

add_compile_options("$<$<CONFIG:DEBUG>:${DEBUG_CC_OPTIONS}>")
add_compile_options("$<$<CONFIG:RELEASE>:${RELEASE_CC_OPTIONS}>")
# add_link_options("LINKER:-fsanitize=address")

macro(define_jcc_lib lib)
  file(GLOB SOURCES *.cpp)
  add_library(${lib} STATIC ${SOURCES})
  set_target_properties(${lib} PROPERTIES PREFIX "libjcc")
  target_include_directories(${lib} PUBLIC "${PROJECT_SOURCE_DIR}/include")
endmacro()

macro(define_jcc_llvm_lib lib)
  define_jcc_lib(${lib})
  target_include_directories(${lib} SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}")
endmacro()

# Sources
add_subdirectory(src)

# Unit tests
enable_testing()
add_subdirectory(test)
