set(TEST_MAIN TestMain)
set(CATCH_INCLUDES "${CMAKE_SOURCE_DIR}/3p/Catch2/single_include")
add_library(${TEST_MAIN}_lib ${TEST_MAIN}.cpp)
target_include_directories(${TEST_MAIN}_lib PUBLIC ${CATCH_INCLUDES})

file(GLOB TEST_SOURCES LIST_DIRECTORIES false *.cpp)
list(REMOVE_ITEM TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_MAIN}.cpp)

foreach (TESTFILE ${TEST_SOURCES})
  string(REPLACE ".cpp" "" TEST ${TESTFILE})
  get_filename_component(UNIT_TEST ${TEST} NAME)

  add_executable(${UNIT_TEST} ${TESTFILE})
  target_include_directories(${UNIT_TEST} PUBLIC ${CATCH_INCLUDES})
  target_include_directories(${UNIT_TEST} PUBLIC "${PROJECT_SOURCE_DIR}/include")
  target_include_directories(${UNIT_TEST} SYSTEM PRIVATE "${LLVM_INCLUDE_DIRS}")
  target_link_libraries(${UNIT_TEST} PUBLIC ${JCC_LIB} ${TEST_MAIN}_lib ${LLVM_LIBS})
  add_test(NAME ${UNIT_TEST} COMMAND ${UNIT_TEST})
endforeach()
