cmake_minimum_required(VERSION 3.5)

set(PROJECT_NAME media_server)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_COMPILER g++)

message(STATUS "cmake ${PROJECT_NAME_BIZ}")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -Wall -g -rdynamic -D_DEBUG -DDEBUG=1 ${DEFS}")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS}  ${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall -DNDEBUG ${DEFS}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libdash/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libflv/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libhls/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libmkv/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libmov/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libmpeg/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/librtmp/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/librtp/include)

#递归搜索当前文件夹下的所有cpp文件
file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/libdash/src/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libflv/source/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libhls/source/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmkv/src/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmov/source/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libmpeg/source/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/librtmp/source/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/librtp/source/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/librtp/payload/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/librtp/rtpext/*.c
)

set(Install_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/linux_lib/)
set(BIN_DIR ${Install_DIR})
set(LIBRARY_OUTPUT_PATH ${BIN_DIR})

link_directories(${LIB_DIR})
find_library(LIBRT rt)
set(LIB ${LIBRT} pthread dl)
message(STATUS "${PROJECT_NAME} LIB: ${LIB}")

add_library(${PROJECT_NAME} SHARED ${SRC})

target_link_libraries(${PROJECT_NAME} PUBLIC ${LIB})
#set_target_properties(${PROJECT_NAME_BIZ} PROPERTIES LINK_FLAGS_RELEASE -s)

# remove related files while make clean
set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${ADDITIONAL_MAKE_CLEAN_FILES_LIST})

#install(TARGETS ${PROJECT_NAME_BIZ} LIBRARY DESTINATION ${Install_DIR})