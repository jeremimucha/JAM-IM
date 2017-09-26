# Top level tests
cmake_minimum_required(VERSION 3.1.0)

set( TEST_PROJECT_NAME MainTests )
set( PROJECT_TESTS_SOURCE MainTests.cpp )


set( CMAKE_CXX_STANDARD 14 )

project( ${TEST_PROJECT_NAME} LANGUAGES CXX )

enable_testing()

add_executable( ${TEST_PROJECT_NAME} ${PROJECT_TESTS_SOURCE} )
target_link_libraries( ${TEST_PROJECT_NAME} 
                       gtest 
                       gtest_main 
                       ${LIBRARIES}        # NOTE: This is defined from project above
                       ${PROJECT_EXT_LIBS} # NOTE: This is defined from project above
                      #  Threads::Threads
                     )
add_test(  ${TEST_PROJECT_NAME} ${TEST_PROJECT_NAME} )
