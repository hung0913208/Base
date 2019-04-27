cmake_minimum_required(VERSION 2.4 FATAL_ERROR)
project(Builder LANGUAGES CXX C)

# @NOTE: add our custom module
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../CMakeModules)

# @NOTE: many CI services still support c++ 11 and some Linux distribute still only
# use C++11 on its main stream so we must consider using it while waiting our repo
# will be developed
include(AddCXXCompilerFlag)
include(AddCCompilerFlag)

add_cxx_compiler_flag(-std=c++11)
add_c_compiler_flag(-std=c11)

if (NOT HAVE_CXX_FLAG_STD_CXX11)
	add_cxx_compiler_flag(-std=c++0x)
endif()

# @NOTE: these common flags will be used globally around my repos
add_cxx_compiler_flag(-fPIC)
add_cxx_compiler_flag(-Wall)
add_cxx_compiler_flag(-Wextra)
add_cxx_compiler_flag(-Wformat=2)
add_cxx_compiler_flag(-fno-strict-aliasing)

# @NOTE: maybe there was a bug inside gcc's stack protector that cause hanging
#  issue on Release mode
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_cxx_compiler_flag(-fno-stack-protector)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
	include(CodeCoverage)

	add_cxx_compiler_flag(-Werror)
	add_cxx_compiler_flag(-Wfatal-errors)
	add_cxx_compiler_flag(-fprofile-arcs)
	add_cxx_compiler_flag(-ftest-coverage)

	add_c_compiler_flag(-Werror)
	add_c_compiler_flag(-Wfatal-errors)
	add_c_compiler_flag(-pedantic)
	add_c_compiler_flag(-pedantic-errors)
	add_c_compiler_flag(-fprofile-arcs)
	add_c_compiler_flag(-ftest-coverage)

	setup_target_for_coverage(NAME ${PROJECT_NAME}_coverage EXECUTABLE tests/alltests)
elseif(CMAKE_BUILD_TYPE STREQUAL "Profiling")
	add_cxx_compiler_flag(-fprofile-generate)
	add_c_compiler_flag(-fprofile-generate)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
	set(CMAKE_CXX_FLAGS_RELEASE "")

	add_cxx_compiler_flag(-O3)
	add_c_compiler_flag(-O3)
	add_cxx_compiler_flag(-g3)
	add_c_compiler_flag(-g3)
	add_cxx_compiler_flag(-fno-unwind-tables)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_cxx_compiler_flag(-Werror)
	add_cxx_compiler_flag(-Wfatal-errors)
	add_cxx_compiler_flag(-fno-stack-protector)

	add_c_compiler_flag(-Werror)
	add_c_compiler_flag(-Wfatal-errors)
endif()
# @NOTE: suppress some feature according to the compilers
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.2)
		add_cxx_compiler_flag(-Wno-format-nonliteral)
	endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	add_cxx_compiler_flag(-Wno-unused-but-set-parameter)
	add_cxx_compiler_flag(-Wno-unused-but-set-variable)
	add_cxx_compiler_flag(-Wno-unused-variable)
	add_cxx_compiler_flag(-Wno-unused-parameter)
	add_cxx_compiler_flag(-Wno-unused-function)
	add_cxx_compiler_flag(-fno-asynchronous-unwind-tables)

	if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
		add_cxx_compiler_flag(-Wno-implicit-fallthrough)
	endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
	# using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	# using Visual Studio C++
endif()

# @NOTE: load essential libraries from here most of them are tested
# also provided test cases so we will build these test cases too for
# testing
include_directories(${CMAKE_SOURCE_DIR}
                    ${CMAKE_SOURCE_DIR}/Libraries/Base
                    ${CMAKE_SOURCE_DIR}/Libraries/Base/Type
                    ${CMAKE_SOURCE_DIR}/Libraries/Data
                    ${CMAKE_SOURCE_DIR}/Libraries/Parallel
                    ${CMAKE_SOURCE_DIR}/Libraries/Wrapping
                    ${CMAKE_SOURCE_DIR}/Libraries/Wrapping/Implement
                    ${CMAKE_SOURCE_DIR}/Libraries/Wrapping/Implement/Javascript
                    ${CMAKE_SOURCE_DIR}/Libraries/Wrapping/Implement/Python
                    ${CMAKE_SOURCE_DIR}/Libraries/Wrapping/Implement/Lua
                    ${CMAKE_SOURCE_DIR}/Libraries/Unittest
                    ${CMAKE_SOURCE_DIR}/Libraries/Unittest/Implement)

add_subdirectory(${CMAKE_SOURCE_DIR}/Libraries/Base)
add_subdirectory(${CMAKE_SOURCE_DIR}/Libraries/Data)
add_subdirectory(${CMAKE_SOURCE_DIR}/Libraries/Unittest)
add_subdirectory(${CMAKE_SOURCE_DIR}/Libraries/Wrapping)

# @NOTE: define how to build project Builder
file(GLOB HEADERS "${CMAKE_SOURCE_DIR}/*.h")
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/Source/*.cc")

find_package(Threads)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif()

add_executable(build2 ${SOURCES})
if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(build2 PUBLIC base -pthread)
elseif(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(build2 base data ${CMAKE_THREAD_LIBS_INIT})
else()
	target_link_libraries(build2 base data -lpthread)
endif()

# @NOTE: build test cases from here
enable_testing()
add_subdirectory(${CMAKE_SOURCE_DIR}/Tests)

add_test(NAME auto COMMAND Tests/auto)
add_test(NAME argparse COMMAND Tests/argparse)
add_test(NAME deadlock COMMAND Tests/deadlock)
add_test(NAME exception COMMAND Tests/exception)
add_test(NAME hashtable COMMAND Tests/hashtable)
add_test(NAME glob COMMAND Tests/glob)
add_test(NAME logcat COMMAND Tests/logcat)
add_test(NAME property COMMAND Tests/property)
add_test(NAME stream COMMAND Tests/stream)
add_test(NAME string COMMAND Tests/string)
add_test(NAME thread COMMAND Tests/thread)
add_test(NAME vertex COMMAND Tests/vertex)
add_test(NAME memmap COMMAND Tests/memmap)
add_test(NAME json COMMAND Tests/json)
add_test(NAME popen COMMAND Tests/popen)

#add_test(NAME hashtable_dbg COMMAND ${CMAKE_SOURCE_DIR}/Tools/Utilities/valgrind.sh Tests/hashtable)
#add_test(NAME json_dbg COMMAND ${CMAKE_SOURCE_DIR}/Tools/Utilities/valgrind.sh Tests/json)
#add_test(NAME deadlock_dbg COMMAND ${CMAKE_SOURCE_DIR}/Tools/Utilities/helgrind.sh Tests/deadlock)

set_tests_properties(hashtable PROPERTIES TIMEOUT 30)
set_tests_properties(deadlock PROPERTIES TIMEOUT 80)
set_tests_properties(exception PROPERTIES TIMEOUT 20)
set_tests_properties(property PROPERTIES TIMEOUT 10)
set_tests_properties(vertex PROPERTIES TIMEOUT 10)
set_tests_properties(logcat PROPERTIES TIMEOUT 20)
set_tests_properties(thread PROPERTIES TIMEOUT 120)
set_tests_properties(json PROPERTIES TIMEOUT 20)
set_tests_properties(popen PROPERTIES TIMEOUT 30)

