file(GLOB TEST_SOURCES LIST_DIRECTORIES false *.cpp)

foreach (TESTFILE ${TEST_SOURCES})
  string(REPLACE ".cpp" "" TEST ${TESTFILE})
  get_filename_component(UNIT_TEST ${TEST} NAME)

  add_executable(${UNIT_TEST} ${TESTFILE})
  target_include_directories(${UNIT_TEST} PUBLIC "${PROJECT_SOURCE_DIR}/include")
  target_include_directories(${UNIT_TEST} SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}")
  target_link_libraries(${UNIT_TEST} PUBLIC ${JCC_LIB} gtest_main ${llvm_libs})
  add_test(NAME ${UNIT_TEST} COMMAND ${UNIT_TEST})
endforeach()
