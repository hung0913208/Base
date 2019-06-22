include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/Implement
		    ${CMAKE_CURRENT_SOURCE_DIR}/../Base
		    ${CMAKE_CURRENT_SOURCE_DIR}/../Base/Type)

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
                  "${CMAKE_CURRENT_SOURCE_DIR}/Implement/*.h")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cc"
                  "${CMAKE_CURRENT_SOURCE_DIR}/Source/Internal/*.cc")

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif()

add_library(unittest STATIC ${SOURCES})
if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	set_source_files_properties(SOURCE ${SOURCES} PROPERTIES COMPILE_FLAGS -O0)
endif()

find_package(Threads)

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(unittest PUBLIC base -pthread)
elseif(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(unittest base ${CMAKE_THREAD_LIBS_INIT})
else()	
	target_link_libraries(unittest base -lpthread)
endif()
