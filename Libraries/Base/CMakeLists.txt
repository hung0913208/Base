include_directories(${CMAKE_CURRENT_SOURCE_DIR}
		    ${CMAKE_SOURCE_DIR}/Libraries)

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
		  "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB DEBUGS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Debug/*.cc")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cc"
		  "${CMAKE_CURRENT_SOURCE_DIR}/Source/Debug/*.cc"
		  "${CMAKE_CURRENT_SOURCE_DIR}/Source/Internal/*.cc"
		  "${CMAKE_CURRENT_SOURCE_DIR}/Source/Internal/*.c")

add_library(base STATIC ${SOURCES})
find_package(Threads)

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(base PUBLIC -pthread)
elseif(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(base ${CMAKE_THREAD_LIBS_INIT})
else()
	target_link_libraries(base -lpthread)
endif()
