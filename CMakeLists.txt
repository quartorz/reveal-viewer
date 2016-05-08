cmake_minimum_required(VERSION 3.0)

project(reveal-viewer)

set(
  SOURCES
  src/main.cpp
  src/main_window.hpp
  src/browser_window.hpp
  src/msr.hpp
  src/utility.hpp
  src/browser_handler.hpp
  src/renderer_handler.hpp
  src/other_handler.hpp
  src/v8_handler.hpp
)

set(Boost_USE_STATIC_LIBS ON)

find_package(Boost)

if( Boost_FOUND )
  include_directories(${Boost_INCLUDE_DIRS})
else()
  message( WARNING
    "Boost not found. "
    "Please specify Boost include directories and library directories manually."
  )
endif()

if( NOT DEFINED QUOTE_PATH )
  message( WARNING
    "QUOTE_PATH is not defined. "
    "Use QUOTE_PATH=${CMAKE_SOURCE_DIR}/quote/include as default."
  )
  set( QUOTE_PATH ${CMAKE_SOURCE_DIR}/quote/include )
endif()

if( DEFINED CEF_PATH )
  set( CEF_INCLUDE_PATH ${CEF_PATH}/include )
  set( CEF_LIBRARY_PATH ${CEF_PATH}/lib )
  set( CEF_BINARY_PATH ${CEF_PATH}/bin )

  message( STATUS
    "CEF_PATH is defined. "
    "Assume that CEF headers are in ${CEF_INCLUDE_PATH}, "
    "CEF libraries are in ${CEF_LIBRARY_PATH}, "
    "and CEF binaries are in ${CEF_BINARY_PATH}."
  )
else()
  if( NOT DEFINED CEF_INCLUDE_PATH )
    message( STATUS
      "CEF_INCLUDE_PATH is not defined. "
      "Use CEF_INCLUDE_PATH=${CMAKE_SOURCE_DIR}/cef/include as default."
    )
    set( CEF_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/cef/include )
  endif()

  if( NOT DEFINED CEF_LIBRARY_PATH )
    message( STATUS
      "CEF_LIBRARY_PATH is not defined. "
      "Use CEF_LIBRARY_PATH=${CMAKE_SOURCE_DIR}/cef/lib as default."
    )
    set( CEF_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/cef/lib )
  endif()

  if( NOT DEFINED CEF_BINARY_PATH )
    message( STATUS
      "CEF_BINARY_PATH is not defined. "
      "Use CEF_LIBRARY_PATH=${CMAKE_SOURCE_DIR}/cef/bin as default."
    )
    set( CEF_BINARY_PATH ${CMAKE_SOURCE_DIR}/cef/bin )
  endif()
endif()

include_directories(${QUOTE_PATH})
link_directories(${CEF_LIBRARY_PATH})

if( Boost_FOUND )
  if( NOT Boost_LIBRARY_DIRS )
    if ( DEFINED BOOST_LIBRARYDIR )
      message( STATUS
        "Boost was found, "
        "but Boost library directories were not found. "
        "Use BOOST_LIBRARYDIR instead."
      )
      link_directories(${BOOST_LIBRARYDIR})
    else()
      message( WARNING
        "Boost was found, "
        "but Boost library directories were not found. "
        "Please specify them manually."
      )
    endif()
  else()
    link_directories(${Boost_LIBRARY_DIRS})
    message("${Boost_LIBRARY_DIRS}")
  endif()
endif()

add_executable(reveal-viewer WIN32 ${SOURCES})

add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

file( GLOB CEF_HEADERS ${CEF_INCLUDE_PATH}/* )
file( COPY ${CEF_HEADERS} DESTINATION ${CMAKE_SOURCE_DIR}/src/include )

target_link_libraries(
  reveal-viewer
  debug Debug/cef_sandbox
  debug Debug/libcef
  debug Debug/libcef_dll_wrapper
  optimized Release/cef_sandbox
  optimized Release/libcef
  optimized Release/libcef_dll_wrapper
)

file( GLOB CEF_DEBUG_BINARIES ${CEF_BINARY_PATH}/Debug/* )
file( GLOB CEF_RELEASE_BINARIES ${CEF_BINARY_PATH}/Release/* )

message(STATUS "Copying CEF binaries.")

file(
  COPY
  ${CEF_DEBUG_BINARIES}
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}/Debug
)

file(
  COPY
  ${CEF_DEBUG_BINARIES}
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo
)

file(
  COPY
  ${CEF_RELEASE_BINARIES}
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}/Release
)

file(
  COPY
  ${CEF_RELEASE_BINARIES}
  DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel
)

foreach( v
  CMAKE_CXX_FLAGS_DEBUG
  CMAKE_CXX_FLAGS_MINSIZEREL
  CMAKE_CXX_FLAGS_RELEASE
  CMAKE_CXX_FLAGS_RELWITHDEBINFO
)
  if( ${v} MATCHES "/MD" )
    string( REGEX REPLACE "/MD" "/MT" ${v} "${${v}}" )
  endif()
endforeach()