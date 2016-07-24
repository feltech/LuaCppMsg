cmake_minimum_required(VERSION 2.8)

project(LuaCppMsg)

cmake_policy(SET CMP0015 NEW)

set(LUA_INCLUDE_DIR /usr/include CACHE PATH "Lua include directory") 
set(LUA_LIB_DIR /usr/include CACHE PATH "LuaJIT static library directory") 

set(PROJECT_ROOT ${PROJECT_SOURCE_DIR})
set(SRC_ROOT ${PROJECT_SOURCE_DIR}/src)
set(TESTS_ROOT ${SRC_ROOT}/tests)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_ROOT}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_ROOT}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_ROOT}/bin)

include_directories(
    ${PROJECT_ROOT}/include ${EIGEN3_INCLUDE_DIR}
)

add_definitions(
   -std=c++14 -Wfatal-errors -fdiagnostics-color=auto -DCATCH_CONFIG_CONSOLE_WIDTH=200 
)

# C++ unit tests

file(GLOB SRC_CPPS "${SRC_ROOT}/*.cpp")
file(GLOB TEST_CPPS "${TESTS_ROOT}/*.cpp")
file(GLOB BIND_CPPS "${SRC_ROOT}/bind/*.cpp")
include_directories(${PROJECT_ROOT}/include ${LUA_INCLUDE_DIR})
link_directories(${LUA_LIB_DIR})
add_executable(tests ${SRC_CPPS} ${TEST_CPPS} ${BIND_CPPS})
target_link_libraries(tests LuaJIT dl pthread)