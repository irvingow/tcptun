cmake_minimum_required(VERSION 3.3)
project(tcptun)

set(CMAKE_CXX_STANDARD 14)

#set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 ")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CMAKE_CXX_FLAGS} -DDEBUG -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CMAKE_CXX_FLAGS} -O3")

include_directories(./include/)
#include_directories(./lib/)
include_directories(./thirdparty/)
#include_directories(/home/lwj/Documents/installed/boost/include/)

#link_directories("/home/lwj/Documents/installed/boost/lib")
#aux_source_directory(./lib/ lib_source_list)
link_libraries("glog")

aux_source_directory(./source/ source_list)

add_executable(tcptun_client samples/tcptun_client.cpp ${source_list} ${lib_source_list} )
add_executable(tcptun_server samples/tcptun_server.cpp ${source_list} ${lib_source_list})

#file(GLOB_RECURSE mains RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/samples/*.cpp")
#foreach(mainfile IN LISTS mains)
#     Get file name without directory
#    get_filename_component(mainname ${mainfile} NAME_WE)
#    add_executable(${mainname} ${mainfile} ${source_list})
#endforeach()
