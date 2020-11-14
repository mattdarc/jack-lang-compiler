add_executable(${COMPILER} Compiler.cpp)
target_link_libraries(${COMPILER} PUBLIC ${JCC_LIB} ${LLVM_LIBS})
target_include_directories(${COMPILER} PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(${COMPILER} SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}")

add_executable(${INTERPRETER} Interpreter.cpp)
target_link_libraries(${INTERPRETER} PUBLIC ${JCC_LIB} ${LLVM_LIBS})
target_include_directories(${INTERPRETER} PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(${INTERPRETER} SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}")
