include_directories(${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../Base
		${CMAKE_CURRENT_SOURCE_DIR}/../Base/Type
		${CMAKE_CURRENT_SOURCE_DIR}/../Data
		${CMAKE_CURRENT_SOURCE_DIR}/../Wrapping
		${CMAKE_CURRENT_SOURCE_DIR}/../Wrapping/Implement)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif()

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DEBUGS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Debug/*.cc")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cc"
		  "${CMAKE_CURRENT_SOURCE_DIR}/Source/Instruct/*.c"
		  "${CMAKE_CURRENT_SOURCE_DIR}/Source/Instruct/*.cc"
		  "${CMAKE_CURRENT_SOURCE_DIR}/Source/Internale/*.c"
		  "${CMAKE_CURRENT_SOURCE_DIR}/Source/Internal/*.cc")

find_package(Threads)
find_package(PythonLibs)

set(LANGUAGE_LIBRARIES "")
if (PYTHONLIBS_FOUND)
	include_directories(${PYTHON_INCLUDE_PATH} ${PYTHON_INCLUDE_DIRS})
	add_definitions(-DUSE_PYTHON=1)

	# @NOTE: since we will use variable LANGUAGE_LIBRAIRES as the collector to collect
	# libraries from upper languages
	set(LANGUAGE_LIBRARIES ${LANGUAGE_LIBRARIES} ${PYTHONLIBS_LIBRARIES})
endif()

add_subdirectory(Source/IBash)
add_subdirectory(Source/IPython)
add_library(wrapping STATIC ${SOURCES})

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	set_source_files_properties(SOURCE ${DEBUGS} PROPERTIES COMPILE_FLAGS -O0)
endif()

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(wrapping PUBLIC base -pthread ${LANGUAGE_LIBRARIES})
elseif(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(wrapping base ${CMAKE_THREAD_LIBS_INIT} ${LANGUAGE_LIBRARIES})
else()
	target_link_libraries(wrapping base -lpthread ${LANGUAGE_LIBRARIES})
endif()

