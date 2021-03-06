project(TIS_NGSIv2)
cmake_minimum_required(VERSION 2.9)

include(CheckCXXCompilerFlag)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG OR
        CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    check_cxx_compiler_flag(--std=c++11 SUPPORTS_CXX11)
    if(SUPPORTS_CXX11)
        add_compile_options(--std=c++11)
    else()
        message(FATAL_ERROR "Compiler doesn't support C++11")
    endif()
endif()

find_package(fastcdr)
find_package(fastrtps)

add_definitions(-DEPROSIMA_USER_DLL_EXPORT  -DBUILD_SHARED_LIBS)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS TRUE)

#file(GLOB USER_LIB_SOURCES_CXX "*.cxx")

#add_executable(tis_ngsiv2 main.cpp)
add_executable(cb_create_robot cb_create_robot.cpp)
add_executable(cb_update_attr cb_update_attr.cpp)
add_executable(RobotExample RobotExample_main.cpp
    RobotExamplePublisher.cpp RobotExampleSubscriber.cpp
    RobotPosition.cxx RobotPositionPubSubTypes.cxx
    RobotRcv.cxx RobotRcvPubSubTypes.cxx
    RobotSnd.cxx RobotSndPubSubTypes.cxx)

add_library(userlib SHARED ${USER_LIB_SOURCES_CXX} userlib.cpp jsoncpp.cpp
    RobotPosition.cxx RobotPositionPubSubTypes.cxx
    RobotRcv.cxx RobotRcvPubSubTypes.cxx
    RobotSnd.cxx RobotSndPubSubTypes.cxx
    ../../src/NGSIv2/idl/JsonNGSIv2.cxx
    ../../src/NGSIv2/idl/JsonNGSIv2PubSubTypes.cxx)
#add_library(userlib2 SHARED ${USER_LIB_SOURCES_CXX} userlib2.cpp jsoncpp.cpp)

target_link_libraries(userlib fastrtps fastcdr ${CMAKE_DL_LIBS})
#target_link_libraries(userlib2 fastrtps fastcdr ${CMAKE_DL_LIBS})
target_link_libraries(cb_create_robot -L/usr/lib/x86_64-linux-gnu curl curlpp)
target_link_libraries(cb_update_attr -L/usr/lib/x86_64-linux-gnu curl curlpp)
target_link_libraries(RobotExample fastrtps fastcdr)
