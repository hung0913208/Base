include_directories(${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../Base
		${CMAKE_CURRENT_SOURCE_DIR}/../Base/Type)

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cc")

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

add_library(data STATIC ${SOURCES})
target_link_libraries(data base)
