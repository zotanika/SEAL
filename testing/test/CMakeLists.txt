cmake_minimum_required(VERSION 3.5)
project(test VERSION 0.1 LANGUAGES C)

set(ROOT_DIR "${CMAKE_SOURCE_DIR}/../..")
set(HEDGE_SRC_DIR "${CMAKE_SOURCE_DIR}/../../hedge")
set(HEDGE_LIB_DIR "${ROOT_DIR}/release")

#execute_process(COMMAND "${ROOT_DIR}/make-hedgelib.sh")

set(CMAKE_BUILD_TYPE Debug)
include(${ROOT_DIR}/hedge/compiler.cmake)

add_executable(hedgetest test.c)
include_directories("${HEDGE_LIB_DIR}/include")
target_link_libraries (hedgetest "${HEDGE_LIB_DIR}/libhedge.a")
target_link_libraries (hedgetest m)
target_link_libraries (hedgetest dl)
