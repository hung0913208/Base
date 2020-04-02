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

# @NOTE: define test cases from Base
add_executable(auto ${CMAKE_CURRENT_SOURCE_DIR}/Base/Auto.cc)
add_executable(argparse ${CMAKE_CURRENT_SOURCE_DIR}/Base/Argparse.cc)
add_executable(deadlock ${CMAKE_CURRENT_SOURCE_DIR}/Base/Deadlock.cc)
add_executable(exception ${CMAKE_CURRENT_SOURCE_DIR}/Base/Exception.cc)
add_executable(glob ${CMAKE_CURRENT_SOURCE_DIR}/Base/Glob.cc)
add_executable(hashtable ${CMAKE_CURRENT_SOURCE_DIR}/Base/Hashtable.cc)
add_executable(lock ${CMAKE_CURRENT_SOURCE_DIR}/Base/Lock.cc)
add_executable(logcat ${CMAKE_CURRENT_SOURCE_DIR}/Base/Logcat.cc)
add_executable(property ${CMAKE_CURRENT_SOURCE_DIR}/Base/Property.cc)
add_executable(protect ${CMAKE_CURRENT_SOURCE_DIR}/Base/Protect.cc)
add_executable(popen ${CMAKE_CURRENT_SOURCE_DIR}/Base/Popen.cc)
add_executable(refcount ${CMAKE_CURRENT_SOURCE_DIR}/Base/Refcount.cc)
add_executable(stream ${CMAKE_CURRENT_SOURCE_DIR}/Base/Stream.cc)
add_executable(string ${CMAKE_CURRENT_SOURCE_DIR}/Base/String.cc)
add_executable(thread ${CMAKE_CURRENT_SOURCE_DIR}/Base/Thread.cc)
add_executable(vertex ${CMAKE_CURRENT_SOURCE_DIR}/Base/Vertex.cc)

target_link_libraries(auto base unittest)
target_link_libraries(argparse base unittest)
target_link_libraries(deadlock base unittest)
target_link_libraries(exception base unittest)
target_link_libraries(glob base unittest)
target_link_libraries(hashtable base unittest)
target_link_libraries(lock base unittest)
target_link_libraries(logcat base unittest)
target_link_libraries(protect base unittest)
target_link_libraries(property base unittest)
target_link_libraries(popen base unittest)
target_link_libraries(refcount base unittest)
target_link_libraries(stream base unittest)
target_link_libraries(string base unittest)
target_link_libraries(thread base unittest)
target_link_libraries(vertex base unittest)

# @NOTE: define test cases from Data
add_executable(memmap ${CMAKE_CURRENT_SOURCE_DIR}/Data/Memmap.cc)
add_executable(json ${CMAKE_CURRENT_SOURCE_DIR}/Data/Json.cc)

target_link_libraries(memmap base data unittest)
target_link_libraries(json base data unittest)

if (NOT BUILD_STATIC_EXEC AND NOT CMAKE_BUILD_TYPE STREQUAL "Coverage" AND NOT CMAKE_BUILD_TYPE STREQUAL "Profiling")
	# @NOTE: define test cases from Wrapping
	add_executable(function ${CMAKE_CURRENT_SOURCE_DIR}/Wrapping/Function.cc)
	add_library(hello SHARED ${CMAKE_CURRENT_SOURCE_DIR}/Wrapping/Hello.cc)

	target_link_libraries(function base data unittest wrapping ${LANGUAGE_LIBRARIES})
	target_link_libraries(hello base wrapping unittest ${LANGUAGE_LIBRARIES})

	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Wrapping/Hello.py 
	     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

add_executable(passed ${CMAKE_CURRENT_SOURCE_DIR}/Unittest/Passed.cc)
target_link_libraries(passed base unittest)
