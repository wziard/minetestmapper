project(MinetestMapper C CXX)
cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0003 NEW)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_FLAGS_RELEASE "-std=c++0x -Wall -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -g -std=c++0x -Wall -Wextra -DDEBUG")

# Find libgd
find_library(LIBGD_LIBRARY gd)
find_path(LIBGD_INCLUDE_DIR gd.h)
message (STATUS "libgd library: ${LIBGD_LIBRARY}")
message (STATUS "libgd headers: ${LIBGD_INCLUDE_DIR}")
if(NOT LIBGD_LIBRARY OR NOT LIBGD_INCLUDE_DIR)
	message(FATAL_ERROR "libgd not found!")
endif(NOT LIBGD_LIBRARY OR NOT LIBGD_INCLUDE_DIR)

# Find zlib
find_library(ZLIB_LIBRARY z)
find_path(ZLIB_INCLUDE_DIR zlib.h)
message (STATUS "zlib library: ${ZLIB_LIBRARY}")
message (STATUS "zlib headers: ${ZLIB_INCLUDE_DIR}")
if(NOT ZLIB_LIBRARY OR NOT ZLIB_INCLUDE_DIR)
	message(FATAL_ERROR "zlib not found!")
endif(NOT ZLIB_LIBRARY OR NOT ZLIB_INCLUDE_DIR)

find_package(PkgConfig)
include(FindPackageHandleStandardArgs)

# Find libsqlite3
pkg_check_modules(PC_LIBSQLITE QUIET sqlite3)
set(LIBSQLITE3_DEFINITIONS ${PC_LIBSQLITE_CFLAGS_OTHER})
find_path(LIBSQLITE3_INCLUDE_DIR sqlite3.h HINTS ${PC_LIBSQLITE_INCLUDEDIR} ${PC_LIBSQLITE_INCLUDE_DIRS} PATH_SUFFIXES libsqlite3)
find_library(LIBSQLITE3_LIBRARY NAMES sqlite3 libsqlite3 HINTS ${PC_LIBSQLITE_LIBDIR} ${PC_LIBSQLITE_LIBRARY_DIRS})

set(LIBSQLITE3_LIBRARIES ${LIBSQLITE3_LIBRARY})
set(LIBSQLITE3_INCLUDE_DIRS ${LIBSQLITE3_INCLUDE_DIR})
find_package_handle_standard_args(LibSqlite3  DEFAULT_MSG LIBSQLITE3_LIBRARY LIBSQLITE3_INCLUDE_DIR)
mark_as_advanced(LIBSQLITE3_INCLUDE_DIR LIBSQLITE3_LIBRARY)

if(NOT LIBSQLITE3_INCLUDE_DIR)
	message(FATAL_ERROR "Could not find sqlite3")
endif(NOT LIBSQLITE3_INCLUDE_DIR)

# Find leveldb
set(USE_LEVELDB 0)

OPTION(ENABLE_LEVELDB "Enable LevelDB backend")

if(ENABLE_LEVELDB)
	find_library(LEVELDB_LIBRARY leveldb)
	find_path(LEVELDB_INCLUDE_DIR db.h PATH_SUFFIXES leveldb)
	message (STATUS "LevelDB library: ${LEVELDB_LIBRARY}")
	message (STATUS "LevelDB headers: ${LEVELDB_INCLUDE_DIR}")
	if(LEVELDB_LIBRARY AND LEVELDB_INCLUDE_DIR)
		set(USE_LEVELDB 1)
		message(STATUS "LevelDB backend enabled")
		include_directories(${LEVELDB_INCLUDE_DIR})
	else(LEVELDB_LIBRARY AND LEVELDB_INCLUDE_DIR)
		set(USE_LEVELDB 0)
		message(STATUS "LevelDB not found!")
	endif(LEVELDB_LIBRARY AND LEVELDB_INCLUDE_DIR)
endif(ENABLE_LEVELDB)

# Find xxd
find_program(XXD_EXECUTABLE xxd)

if (XXD_EXECUTABLE)
	message(STATUS "Found xxd")
else (XXD_EXECUTABLE)
	message(FATAL_ERROR "xxd not found")
endif (XXD_EXECUTABLE)

# Add command to turn colors.txt into a C header file
add_custom_command(
	OUTPUT colors.h
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/colors.txt colors.txt
	COMMAND xxd -i colors.txt colors.h
	DEPENDS colors.txt
)
set_property(SOURCE TileGenerator.cpp APPEND PROPERTY OBJECT_DEPENDS colors.h)

include_directories(
	"${PROJECT_BINARY_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_BINARY_DIR}"
	${LIBSQLITE3_INCLUDE_DIRS}
	${LIBGD_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
)

configure_file(
	"${PROJECT_SOURCE_DIR}/cmake_config.h.in"
	"${PROJECT_BINARY_DIR}/cmake_config.h"
)
add_definitions ( -DUSE_CMAKE_CONFIG_H )

set(mapper_SRCS
	PixelAttributes.cpp
	PlayerAttributes.cpp
	TileGenerator.cpp
	ZlibDecompressor.cpp
	mapper.cpp
	db-sqlite3.cpp
)

if(USE_LEVELDB)
	set(mapper_SRCS ${mapper_SRCS} db-leveldb.cpp)
endif(USE_LEVELDB)

add_executable(minetestmapper
	${mapper_SRCS}
)

target_link_libraries(
	minetestmapper
	${LIBSQLITE3_LIBRARIES}
	${LEVELDB_LIBRARY}
	${LIBGD_LIBRARY}
	${ZLIB_LIBRARY}
)
