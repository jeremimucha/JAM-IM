
cmake_minimum_required(VERSION 3.1.0)

set( TEST_PROJECT_NAME
     jamimLibraryTests
)
set( LIBRARY_TESTS_SOURCE
     ClientTests.cpp
     ServerTests.cpp
     MessageTests.cpp
)


set( CMAKE_CXX_STANDARD 14 )

project( ${TEST_PROJECT_NAME} LANGUAGES CXX )

enable_testing()


add_executable( ${TEST_PROJECT_NAME} ${LIBRARY_TESTS_SOURCE} )
target_link_libraries( ${TEST_PROJECT_NAME} 
                       gtest
                       gtest_main
                       ${LIBRARY_EXT_LIBS}  # NOTE: This is defined from project above
                    #    Threads::Threads
                       ${LIBRARY_NAME}      # NOTE: This is defined from project above
)
add_test( ${TEST_PROJECT_NAME} ${TEST_PROJECT_NAME} )
