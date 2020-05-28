include_directories(${CMAKE_SOURCE_DIR}/Libraries/Base
		    ${CMAKE_SOURCE_DIR}/Libraries/Base/Type
		    ${CMAKE_SOURCE_DIR}/Libraries/Data
		    ${CMAKE_SOURCE_DIR}/Libraries/Unittest
		    ${CMAKE_SOURCE_DIR}/Libraries/Unittest/Implement)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif()

find_package(Threads)

#add_executable(memmap ${CMAKE_CURRENT_SOURCE_DIR}/Memmap.cc)
#add_executable(json ${CMAKE_CURRENT_SOURCE_DIR}/Json.cc)
#
#target_link_libraries(memmap base data unittest)
#target_link_libraries(json base data unittest)
#
#add_test(NAME memmap COMMAND ${CMAKE_CURRENT_BINARY_DIR}/memmap)
#add_test(NAME json COMMAND ${CMAKE_CURRENT_BINARY_DIR}/json)
#
#set_tests_properties(json PROPERTIES TIMEOUT 20)
