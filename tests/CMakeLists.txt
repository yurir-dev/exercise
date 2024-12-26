cmake_minimum_required(VERSION 3.10)


include_directories(../.)
include_directories(./.)

#include_directories(${CMAKE_SOURCE_DIR} . ../ )

# Files common to all tests
#set (COMMON_SOURCES ../lockfreeQueue.h ./test_common.h ./stdQueueLock.h)

set(TEST_PHILOSOFIERS test_philosofiers)
add_executable(${TEST_PHILOSOFIERS} test_philosofiers.cpp)

set(exes ${TEST_PHILOSOFIERS})

if (UNIX)
message("creating linux project")
foreach (exe IN LISTS exes)
	target_link_libraries(${exe} pthread)
endforeach()
endif()

foreach (exe IN LISTS exes)
	add_test(${exe} ${exe})
endforeach()
