cmake_minimum_required( VERSION 3.20 )

if( "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}" )
    message( FATAL_ERROR ">>> [ERRO] In-source builds are not allowed. You should create a separate directory for build files." )
endif()

project( C2P )


# ============================================================
# build options:
# ============================================================

# specify the C++ standard
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED True )

# for C++ develop tools:
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

# NOTE: Add other build options here.


# ============================================================
# project version cmake vars define:
# ============================================================

# git version tag:
execute_process(
    COMMAND git describe --always --tags --abbrev=8 --match "v[0-9].[0-9].[0-9]"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE PROJECT_GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# git branch name:
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE PROJECT_GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# cmake config time:
string( TIMESTAMP PROJECT_CMAKE_TIME "%Y%m%d-%H:%M:%S" )


# ============================================================
# add targets:
# ============================================================

set( PROJECT_TARGETS "" )

# target: lib c2p
add_library( c2p
    src/json.cpp
    src/ini.cpp
    src/cli.cpp
)
list( APPEND PROJECT_TARGETS c2p )
target_include_directories( c2p PUBLIC include )
target_include_directories( c2p PRIVATE src )

# version and build info:
if( PROJECT_GIT_VERSION )
    target_compile_definitions( c2p PRIVATE PROJECT_GIT_VERSION="${PROJECT_GIT_VERSION}" )
endif()
if( PROJECT_GIT_BRANCH )
    target_compile_definitions( c2p PRIVATE PROJECT_GIT_BRANCH="${PROJECT_GIT_BRANCH}" )
endif()
if( PROJECT_CMAKE_TIME )
    target_compile_definitions( c2p PRIVATE PROJECT_CMAKE_TIME="${PROJECT_CMAKE_TIME}" )
endif()

# target: exe example_transform
add_executable( example_transform example/example_transform.cpp )
list( APPEND PROJECT_TARGETS example_transform )
target_link_libraries( example_transform PRIVATE c2p )

# target: exe example_value_tree
add_executable( example_value_tree example/example_value_tree.cpp )
list( APPEND PROJECT_TARGETS example_value_tree )
target_link_libraries( example_value_tree PRIVATE c2p )

# target: exe example_json
add_executable( example_json example/example_json.cpp )
list( APPEND PROJECT_TARGETS example_json )
target_link_libraries( example_json PRIVATE c2p )


# ============================================================
# print info:
# ============================================================

# project version:
message( STATUS ">>> ----------------------------------------" )
message( STATUS ">>> [INFO] project: ${PROJECT_NAME}" )
if( PROJECT_GIT_VERSION )
    message( STATUS ">>> [INFO] version: ${PROJECT_GIT_VERSION}" )
endif()
if( PROJECT_GIT_BRANCH )
    message( STATUS ">>> [INFO] branch: ${PROJECT_GIT_BRANCH}" )
endif()
if( PROJECT_CMAKE_TIME )
    message( STATUS ">>> [INFO] cmake time: ${PROJECT_CMAKE_TIME}" )
endif()

# targets:
message( STATUS ">>> ----------------------------------------" )
message( STATUS ">>> [INFO] targets:")
foreach( TARGET ${PROJECT_TARGETS} )
    message( STATUS ">>>        * ${TARGET}" )
endforeach()

# compiler flags:
message( STATUS ">>> ----------------------------------------" )
message( STATUS ">>> [INFO] compiler flags:" )
message( STATUS ">>>        CMAKE_BUILD_TYPE         : ${CMAKE_BUILD_TYPE}" )
message( STATUS ">>>        CMAKE_CONFIGURATION_TYPES: ${CMAKE_CONFIGURATION_TYPES}" )
message( STATUS ">>>        CMAKE_C_FLAGS            : ${CMAKE_C_FLAGS}" )
message( STATUS ">>>        CMAKE_CXX_FLAGS          : ${CMAKE_CXX_FLAGS}" )

# build options:
message( STATUS ">>> [INFO] build options:")
message( STATUS ">>>        BUILD_SHARED_LIBS: ${BUILD_SHARED_LIBS}")

# NOTE: Add more CMake log print here.

message( STATUS ">>> ----------------------------------------" )
