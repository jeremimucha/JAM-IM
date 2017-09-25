
cmake_minimum_required(VERSION 3.0.0)

set( PROJECT_NAME
     jamimLibrary
)

set( LIBRARY_NAME
     jamim
)

# Library headers
set( LIBRARY_HEADERS_DIR
     include/${LIBRARY_NAME}
)
file( GLOB LIBRARY_HEADERS
      "${LIBRARY_HEADERS_DIR}/*.hpp"
)

# Library source files
set( LIBRARY_SOURCE_DIR
     src
)
file( GLOB LIBRARY_SOURCE
      "${LIBRARY_SOURCE_DIR}/*.cpp"
)

# External libraries
set( LIBRARY_EXT_LIBS
     boost_system-mt
     boost_regex-mt
     boost_filesystem-mt
     boost_date_time-mt
     boost_thread-mt 
)

if( WIN32 )
    set( LIBRARY_EXT_LIBS ${LIBRARY_EXT_LIBS} ws2_32 wsock32 )
endif( WIN32 )


project( ${PROJECT_NAME} )


add_library( ${LIBRARY_NAME} STATIC
             ${LIBRARY_HEADERS}
             ${LIBRARY_SOURCE}
)

target_include_directories( ${LIBRARY_NAME} PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/${LIBRARY_NAME}>
    $<INSTALL_INTERFACE:include/${LIBRARY_NAME}>
)

target_include_directories( ${LIBRARY_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries( ${LIBRARY_NAME}
                       ${LIBRARY_EXT_LIBS}
                     )

# Build tests
add_subdirectory(test)
