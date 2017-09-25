cmake_minimum_required( VERSION 3.1.0 )

set( PROJECT_NAME
     JAM-IM
)

# Dependencies / headers - 
set( PROJECT_HEADERS_DIR
     include
)
# glob all headers in the include dir
file( GLOB PROJECT_HEADERS
      "${PROJECT_HEADERS_DIR}/*.hpp"
)

# Build the client executable
set( CLIENT_EXECUTABLE_NAME
     Client
)
set( CLIENT_SOURCE_DIR
     src
)
set( CLIENT_SOURCE
     ${CLIENT_SOURCE_DIR}/client.cpp)

# Build the server executable
set( SERVER_EXECUTABLE_NAME
     Server
)
set( SERVER_SOURCE_DIR
     src
)
set( SERVER_SOURCE
     ${SERVER_SOURCE_DIR}/server.cpp)


# Build and link libraries
set( LIBRARIES_DIR
     libs
)
set( LIBRARIES
     jamim
)


set( CMAKE_CXX_STANDARD 14 )

project( ${PROJECT_NAME} LANGUAGES CXX )

# External libraries
set( CMAKE_THREAD_PREFER_PTHREAD TRUE )
set( THREADS_PREFER_PTHREAD TRUE )
find_package( Threads REQUIRED )
set( PROJECT_EXT_LIBS
     boost_system-mt
     boost_regex-mt
     boost_filesystem-mt
     boost_date_time-mt
     boost_thread-mt
     Threads::Threads
)

if( WIN32 )
set( PROJECT_EXT_LIBS ${PROJECT_EXT_LIBS} ws2_32 wsock32 )
endif()

# set compiler flags
set( CXX_FLAGS "-O2 -Wall -Wextra -pedantic -std=c++14" )

enable_testing()


#--- Project --- #
include_directories( ${PROJECT_HEADERS_DIR} )
add_executable( ${CLIENT_EXECUTABLE_NAME} ${CLIENT_SOURCE} )
set_target_properties( ${CLIENT_EXECUTABLE_NAME}
                       PROPERTIES 
                       COMPILE_FLAGS
                       ${CXX_FLAGS}
                     )
target_link_libraries( ${CLIENT_EXECUTABLE_NAME} 
                       ${LIBRARIES}
                       ${PROJECT_EXT_LIBS}
                       Threads::Threads
                     )

add_executable( ${SERVER_EXECUTABLE_NAME} ${SERVER_SOURCE} )
set_target_properties( ${SERVER_EXECUTABLE_NAME}
                       PROPERTIES 
                       COMPILE_FLAGS
                       ${CXX_FLAGS}
                     )
target_link_libraries( ${SERVER_EXECUTABLE_NAME} 
                       ${LIBRARIES}
                       ${PROJECT_EXT_LIBS}
                       Threads::Threads 
                     )

foreach( LIBRARY ${LIBRARIES} )
    add_subdirectory( "${LIBRARIES_DIR}/${LIBRARY}" )
endforeach( LIBRARY )


# add_subdirectory( test )
