cmake_minimum_required( VERSION 3.0.0 )

set( PROJECT_NAME
     jamim
)

# Dependencies / headers - 
set( PROJECT_HEADERS_DIR
     include
)
# glob all headers in the include dir
file( GLOB PROJECT_HEADERS
      "${PROJECT_HEADERS_DIR}/*.hpp"
)
# or declare them explicitly
# set( PROJECT_HEADERS ${PROJECT_HEADERS_DIR}/List.hpp )

# Source files
# set( PROJECT_SOURCE_DIR
#      src
# )
# # glob all source files in the src dir
# file( GLOB PROJECT_SOURCE
#       "${PROJECT_SOURCE_DIR}/*.cpp"
# )
# or declare them explicitly
# set( SOURCE ${PROJECT_SOURCE_DIR}/List.cpp ${PROJECT_SOURCE_DIR}/main.cpp )

# Build and link libraries
set( LIBRARIES_DIR
     libs
)
set( LIBRARIES
     jamim
)

# Link external libraries
set( PROJECT_EXT_LIBS
     boost_system-mt
     boost_regex-mt
     boost_filesystem-mt
     boost_date_time-mt
     boost_thread-mt 
     ws2_32 
     wsock32
)

# set compiler flags
set( CXX_FLAGS "-O2 -Wall -Wextra -pedantic -std=c++14" )


project( ${PROJECT_NAME} )


enable_testing()


#--- Project --- #
include_directories( ${PROJECT_HEADERS_DIR} )
add_executable( Client src/client.cpp )
set_target_properties( Client
                       PROPERTIES 
                       COMPILE_FLAGS
                       ${CXX_FLAGS}
                     )
target_link_libraries( Client ${LIBRARIES} ${PROJECT_EXT_LIBS} )

add_executable( Server src/server.cpp )
set_target_properties( Server
                       PROPERTIES 
                       COMPILE_FLAGS
                       ${CXX_FLAGS}
)
target_link_libraries( Server ${LIBRARIES} ${PROJECT_EXT_LIBS} )

foreach( LIBRARY ${LIBRARIES} )
    add_subdirectory( "${LIBRARIES_DIR}/${LIBRARY}" )
endforeach( LIBRARY )



# add_subdirectory( test )
