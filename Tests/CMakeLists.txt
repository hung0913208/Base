include_directories(${CMAKE_SOURCE_DIR}/Libraries/Base
		    ${CMAKE_SOURCE_DIR}/Libraries/Base/Type
		    ${CMAKE_SOURCE_DIR}/Libraries/Data
		    ${CMAKE_SOURCE_DIR}/Libraries/Unittest
		    ${CMAKE_SOURCE_DIR}/Libraries/Unittest/Implement
		    ${CMAKE_SOURCE_DIR}/Libraries/Wrapping
		    ${CMAKE_SOURCE_DIR}/Libraries/Wrapping/Implement)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif()

find_package(Threads)
find_package(PythonLibs)

set(LANGUAGE_LIBRARIES "")
if (PYTHONLIBS_FOUND)
	include_directories(${PYTHON_INCLUDE_PATH} ${PYTHON_INCLUDE_DIRS})
	add_definitions(-DUSE_PYTHON=1)

	# @NOTE: since we will use variable LANGUAGE_LIBRAIRES as the collector to collect
	# libraries from upper languages
	set(LANGUAGE_LIBRARIES ${LANGUAGE_LIBRARIES} ${PYTHON_LIBRARIES})
endif()

add_subdirectory(Base)
#add_subdirectory(Data)
#add_subdirectory(Unittest)
#
#if (NOT BUILD_STATIC_EXEC AND NOT CMAKE_BUILD_TYPE STREQUAL "Coverage" AND NOT CMAKE_BUILD_TYPE STREQUAL "Profiling")
#	add_subdirectory(Wrapping)
#endif()
