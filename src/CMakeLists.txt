add_library(${JCC_LIB} INTERFACE)
target_link_libraries(${JCC_LIB} INTERFACE codegen runtime frontend)

add_subdirectory(app)
add_subdirectory(codegen)
add_subdirectory(runtime)
add_subdirectory(frontend)
