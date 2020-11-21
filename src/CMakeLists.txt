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
  target_link_libraries(${lib} ${llvm_libs})
endmacro()

add_subdirectory(codegen)
add_subdirectory(runtime)
add_subdirectory(frontend)

add_library(${JCC_LIB} INTERFACE)
target_link_libraries(${JCC_LIB} INTERFACE codegen runtime frontend)

add_subdirectory(app)
