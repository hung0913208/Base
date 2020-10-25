cmake_minimum_required(VERSION 2.4 FATAL_ERROR)
project(LibBase)

option(BUILD_STATIC_EXEC "Build the execution" OFF)
option(BUILD_TESTS "Build the test suite" ON)
option(BUILD_WITH_MEMORY_SANITIZE "Build with memory sanitize" OFF)
option(BUILD_WITH_THREAD_SANITIZE "Build with thread sanitize" OFF)
option(BUILD_WITH_ADDRESS_SANITIZE "Build with address sanitize" ON)

# @NOTE: add our custom module
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (BUILD_STATIC_EXEC)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	set(BUILD_SHARED_LIBS OFF)
	set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()
find_package(GTest)

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
add_cxx_compiler_flag(-Wno-deprecated-register)
add_cxx_compiler_flag(-fno-omit-frame-pointer)

# @NOTE: maybe there was a bug inside gcc's stack protector that cause hanging
#  issue on Release mode
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_cxx_compiler_flag(-fno-stack-protector)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
	include(CodeCoverage)

	add_definitions(-DCOVERAGE=1)

	add_cxx_compiler_flag(-Werror)
	add_cxx_compiler_flag(-O3)
	add_c_compiler_flag(-O3)
	add_cxx_compiler_flag(-g3)
	add_c_compiler_flag(-g3)
	add_cxx_compiler_flag(-Wfatal-errors)
	add_cxx_compiler_flag(-fprofile-arcs)
	add_cxx_compiler_flag(-ftest-coverage)

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
elseif(CMAKE_BUILD_TYPE STREQUAL "Sanitize")
	add_cxx_compiler_flag(-fno-omit-frame-pointer)
	add_c_compiler_flag(-fno-omit-frame-pointer)
	add_cxx_compiler_flag(-fno-unwind-tables)
	add_cxx_compiler_flag(-O1)
	add_c_compiler_flag(-O1)
	add_cxx_compiler_flag(-g3)
	add_c_compiler_flag(-g3)

	if(BUILD_WITH_MEMORY_SANITIZE)
		add_cxx_compiler_flag(-fsanitize=memory)
		add_cxx_compiler_flag(-fsanitize-memory-track-origins=2)
		add_c_compiler_flag(-fsanitize=memory)
		add_c_compiler_flag(-fsanitize-memory-track-origins=2)
	elseif(BUILD_WITH_ADDRESS_SANITIZE)
		add_cxx_compiler_flag(-fsanitize=address)
		add_c_compiler_flag(-fsanitize=address)
	elseif(BUILD_WITH_THREAD_SANITIZE)
		add_cxx_compiler_flag(-fsanitize=thread)
		add_c_compiler_flag(-fsanitize=thread)
	endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
	#add_cxx_compiler_flag(-Werror)
	add_cxx_compiler_flag(-Wfatal-errors)
	add_cxx_compiler_flag(-fno-stack-protector)
endif()

# @NOTE: suppress some feature according to the compilers
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.2)
		add_cxx_compiler_flag(-Wno-format-nonliteral)
	endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
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

add_subdirectory(Libraries/Base)
add_subdirectory(Libraries/Data)
add_subdirectory(Libraries/Unittest)

if (NOT BUILD_STATIC_EXEC)
	add_subdirectory(Libraries/Wrapping)
endif()

if (BUILD_TESTS)
	enable_testing()
	add_subdirectory(Tests)
endif()
