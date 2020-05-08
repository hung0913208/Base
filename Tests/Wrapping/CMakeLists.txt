include_directories(${CMAKE_SOURCE_DIR}/Libraries/Base
		    ${CMAKE_SOURCE_DIR}/Libraries/Base/Type
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

# @NOTE: define test cases from Wrapping
add_executable(function ${CMAKE_CURRENT_SOURCE_DIR}/Function.cc)
add_library(hello SHARED ${CMAKE_CURRENT_SOURCE_DIR}/Hello.cc)

target_link_libraries(function base data unittest wrapping ${LANGUAGE_LIBRARIES})
target_link_libraries(hello base wrapping unittest ${LANGUAGE_LIBRARIES})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Hello.py 
	DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME hello COMMAND bash -c "cd ${CMAKE_CURRENT_BINARY_DIR} && /usr/bin/python3 ./Hello.py")
add_test(NAME function COMMAND ${CMAKE_CURRENT_BINARY_DIR}/function)

set_tests_properties(hello PROPERTIES TIMEOUT 200)
