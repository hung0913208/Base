include_directories(${CMAKE_SOURCE_DIR}/Libraries/Base
		    ${CMAKE_SOURCE_DIR}/Libraries/Base/Type
		    ${CMAKE_SOURCE_DIR}/Libraries/Unittest
		    ${CMAKE_SOURCE_DIR}/Libraries/Unittest/Implement)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif()

find_package(Threads)

add_executable(passed ${CMAKE_CURRENT_SOURCE_DIR}/Passed.cc)
target_link_libraries(passed base unittest)

add_executable(fail ${CMAKE_CURRENT_SOURCE_DIR}/Fail.cc)
target_link_libraries(fail base unittest)

add_test(NAME passed COMMAND ./passed)
add_test(NAME fail COMMAND ./fail)
