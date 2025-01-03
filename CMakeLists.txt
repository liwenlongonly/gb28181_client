cmake_minimum_required (VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)

project(gb28181-client LANGUAGES CXX C)

#Determin target architecture
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TargetArchitecture.cmake)
target_architecture(TARGET_ARCH)
message("####### Target Architecture is ${TARGET_ARCH}")

#configure lib for different architecture
execute_process(COMMAND sh ${PROJECT_SOURCE_DIR}/tools/config_lib.sh ${TARGET_ARCH})


set(VERSION_HEAD_FILE ${PROJECT_SOURCE_DIR}/src/version.h)
execute_process(COMMAND awk /DA_MAJOR_VERSION/ ${VERSION_HEAD_FILE} OUTPUT_VARIABLE VERSION_STR)
string(REGEX MATCH "[0-9]+"  MAJOR_VERSION ${VERSION_STR})
execute_process(COMMAND awk /DA_MINOR_VERSION/ ${VERSION_HEAD_FILE} OUTPUT_VARIABLE VERSION_STR)
string(REGEX MATCH "[0-9]+"  MINOR_VERSION ${VERSION_STR})
execute_process(COMMAND awk /DA_PATCH_VERSION/ ${VERSION_HEAD_FILE} OUTPUT_VARIABLE VERSION_STR)
string(REGEX MATCH "[0-9]+"  PATCH_VERSION ${VERSION_STR})

string(TIMESTAMP COMPILE_TIME %Y%m%d%H%M%S)
message(STATUS "TIMESTAMP: ${COMPILE_TIME}")

set(DA_TARGET_VERSION "v${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}_${COMPILE_TIME}")


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/media-server/libmov/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/media-server/libmpeg/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/media-server/librtp/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdpart/media-server/libflv/include)

file(GLOB SRCS ${PROJECT_SOURCE_DIR}/src/*.cpp
        ${PROJECT_SOURCE_DIR}/src/*.c
        ${PROJECT_SOURCE_DIR}/src/common/*.cpp
        ${PROJECT_SOURCE_DIR}/src/common/*.c
)

set(LIB_DIR ${PROJECT_SOURCE_DIR}/lib/linux_lib)
link_directories(${LIB_DIR})

set(BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin/)
set(EXECUTABLE_OUTPUT_PATH ${BIN_DIR})

add_executable(${PROJECT_NAME} ${SRCS})
target_link_libraries(${PROJECT_NAME} eXosip2 osip2 osipparser2 SQLiteCpp sqlite3
                                 pthread resolv crypto ssl media_server stdc++fs)

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,--disable-new-dtags,-rpath,../linux_lib:./linux_lib")

add_subdirectory(./3rdpart/media-server)
add_subdirectory(./3rdpart/SQLiteCpp)

# remove related files while make clean
set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${ADDITIONAL_MAKE_CLEAN_FILES_LIST})

set(install_DIR ${PROJECT_SOURCE_DIR}/install/${PROJECT_NAME})

#何时执行自定义指令
#PRE_BUILD | PRE_LINK | POST_BUILD
# 编译前copy文件
add_custom_command(
        TARGET ${PROJECT_NAME}  PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/src/cfg.ini
        ${BIN_DIR}/cfg.ini)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${install_DIR}/bin"
)

install(DIRECTORY ${LIB_DIR} DESTINATION ${install_DIR}
        PATTERN "*.sh" EXCLUDE # 剔除文件
)

install(FILES ${PROJECT_SOURCE_DIR}/src/cfg.ini
        DESTINATION "${install_DIR}/bin"
        COMPONENT devel
)

install(FILES
        ${PROJECT_SOURCE_DIR}/tools/config.sh
        ${PROJECT_SOURCE_DIR}/tools/process_manager.sh
        ${PROJECT_SOURCE_DIR}/tools/keepalive.sh

        DESTINATION "${install_DIR}"
        COMPONENT devel
)

install(CODE "execute_process(COMMAND sh ${PROJECT_SOURCE_DIR}/tools/target_rename.sh ${PROJECT_NAME} ${DA_TARGET_VERSION} ${install_DIR}/bin)")
