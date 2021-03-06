cmake_minimum_required(VERSION 2.8.3)

project(firos2)

include(CheckCXXCompilerFlag)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG OR
        CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    check_cxx_compiler_flag(--std=c++14 SUPPORTS_CXX11)
    if(SUPPORTS_CXX11)
        add_compile_options(--std=c++14)
    else()
        message(FATAL_ERROR "Compiler doesn't support C++11")
    endif()
endif()

cmake_policy(SET CMP0057 NEW)

find_package(ament_cmake REQUIRED)
find_package(fastrtps REQUIRED)
find_package(fastcdr REQUIRED)

if(WIN32)
	LINK_DIRECTORIES(thirdparty/curl/lib)
endif()

add_executable(firos2 src/main.cpp
    thirdparty/integration-services/src/ISManager.cpp
    thirdparty/integration-services/src/dynamicload/dynamicload.cpp
    thirdparty/integration-services/src/GenericPubSubTypes.cpp)

add_library(isngsiv2rtpsbridgelib SHARED
    src/NGSIv2/ISBridgeNGSIv2ToFastRTPS.cpp
    src/NGSIv2/ngsiv2rtpslib.cpp
    src/NGSIv2/idl/JsonNGSIv2.cxx
    src/NGSIv2/idl/JsonNGSIv2PubSubTypes.cxx
    thirdparty/integration-services/src/dynamicload/dynamicload.cpp
    thirdparty/integration-services/src/GenericPubSubTypes.cpp)

add_library(isrtpsngsiv2bridgelib SHARED
    src/NGSIv2/ISBridgeFastRTPSToNGSIv2.cpp
    src/NGSIv2/rtpsngsiv2lib.cpp
    src/NGSIv2/idl/JsonNGSIv2.cxx
    src/NGSIv2/idl/JsonNGSIv2PubSubTypes.cxx
    thirdparty/integration-services/src/dynamicload/dynamicload.cpp
    thirdparty/integration-services/src/GenericPubSubTypes.cpp)

# Include thirdparty (ASIO only in WIN32)
if (WIN32)
    add_definitions(-DASIO_STANDALONE)
    target_include_directories(firos2 PUBLIC thirdparty/tinyxml2 thirdparty/curlpp/include thirdparty/asio/asio/include)
    target_include_directories(isngsiv2rtpsbridgelib PUBLIC thirdparty/tinyxml2 thirdparty/curlpp/include thirdparty/asio/asio/include)
    target_include_directories(isrtpsngsiv2bridgelib PUBLIC thirdparty/tinyxml2 thirdparty/curlpp/include thirdparty/asio/asio/include)
else()
    target_include_directories(firos2 PUBLIC thirdparty/tinyxml2 thirdparty/curlpp/include)
    target_include_directories(isngsiv2rtpsbridgelib PUBLIC thirdparty/tinyxml2 thirdparty/curlpp/include)
    target_include_directories(isrtpsngsiv2bridgelib PUBLIC thirdparty/tinyxml2 thirdparty/curlpp/include)
endif()

if (WIN32)
	# TinyXML2 as thirdparty
	add_library(TinyXML2 thirdparty/tinyxml2/tinyxml2.cpp thirdparty/tinyxml2/tinyxml2.h)

	# cURLpp as thirdparty
	file(GLOB_RECURSE CURLPP_HEADERS thirdparty/curlpp/include/*)
	file(GLOB_RECURSE CURLPP_SOURCES thirdparty/curlpp/src/*)

	add_library(cURLpp ${CURLPP_HEADERS} ${CURLPP_SOURCES})
    target_include_directories(cURLpp PUBLIC thirdparty/curlpp/include thirdparty/curl/include)
	set_property(TARGET cURLpp PROPERTY POSITION_INDEPENDENT_CODE ON)

    set(liblist fastrtps_static fastcdr_static TinyXML2 ${CMAKE_DL_LIBS} cURLpp IPHLPAPI shlwapi libcurl_a)
else()
    set(liblist fastrtps fastcdr -L/usr/lib/x86_64-linux-gnu tinyxml2 curl curlpp)
endif()

target_link_libraries(firos2 ${CMAKE_DL_LIBS} ${liblist})

target_link_libraries(isngsiv2rtpsbridgelib
    ${liblist}
    ${CMAKE_DL_LIBS})

target_link_libraries(isrtpsngsiv2bridgelib
    ${liblist}
    ${CMAKE_DL_LIBS})

ament_export_dependencies(rosidl_cmake)
ament_export_dependencies(rosidl_generator_dds_idl)

if(BUILD_TESTING)
    find_package(rclcpp REQUIRED)
    find_package(std_msgs REQUIRED)
    find_package(PythonInterp 3)

    add_definitions(-DEPROSIMA_USER_DLL_EXPORT -Duserlib_EXPORTS -DBUILD_SHARED_LIBS)

    add_library(Test SHARED test/Test.cpp
        test/TestPubSubTypes.cpp)
    add_library(String SHARED  test/String_.cpp
        test/String_PubSubTypes.cpp)

    target_link_libraries(Test fastrtps fastcdr)
    target_link_libraries(String fastrtps fastcdr)

    add_executable(test_firos2_fastrtps_subscriber test/fastrtps_subscriber.cpp)
    target_link_libraries(  test_firos2_fastrtps_subscriber
                            Test
                            fastrtps
                            fastcdr)

    add_executable(test_firos2_ros2_publisher test/ros2_publisher.cpp)
    ament_target_dependencies(test_firos2_ros2_publisher rclcpp std_msgs)

    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(BUILD_SHARED_LIBS TRUE)

    add_library(Transformation SHARED test/Transformation.cpp)
    target_link_libraries(  Transformation
                            Test
                            String
                            fastrtps
                            fastcdr
                            ${CMAKE_DL_LIBS})

    if(PYTHONINTERP_FOUND)
        ament_add_test(
            "ros2_to_fastrtps"
            COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test/ros2_to_fastrtps.py
            GENERATE_RESULT_FOR_RETURN_CODE_ZERO
        )

        # Set test with label NoMemoryCheck
        set_property(TEST ros2_to_fastrtps PROPERTY LABELS "NoMemoryCheck")

        #if(WIN32)
        #    set_property(TEST ros2_to_fastrtps PROPERTY ENVIRONMENT
        #        "PATH=$<TARGET_FILE_DIR:${PROJECT_NAME}>\\;$<TARGET_FILE_DIR:fastcdr>\\;${BOOST_LIBRARYDIR}\\;$ENV{PATH}")
        #endif()

        set_property(TEST ros2_to_fastrtps APPEND PROPERTY ENVIRONMENT
            "TEST_FIROS2_BIN_DIR=${CMAKE_CURRENT_BINARY_DIR}")
        set_property(TEST ros2_to_fastrtps APPEND PROPERTY ENVIRONMENT
            "TEST_FIROS2_ROS2_PUBLISHER_BIN=$<TARGET_FILE:test_firos2_ros2_publisher>")
        set_property(TEST ros2_to_fastrtps APPEND PROPERTY ENVIRONMENT
            "TEST_FIROS2_FASTRTPS_SUBSCRIBER_BIN=$<TARGET_FILE:test_firos2_fastrtps_subscriber>")
        set_property(TEST ros2_to_fastrtps APPEND PROPERTY ENVIRONMENT
            "TEST_FIROS2_BIN=$<TARGET_FILE:firos2>")
        set_property(TEST ros2_to_fastrtps APPEND PROPERTY ENVIRONMENT
            "TEST_FIROS2_CONFIG_XML_EM=${CMAKE_CURRENT_SOURCE_DIR}/test/config.xml.em")
        set_property(TEST ros2_to_fastrtps APPEND PROPERTY ENVIRONMENT
            "TEST_TRANSFORMATION_PATH=$<TARGET_FILE:Transformation>")

    endif()
endif()

install(TARGETS firos2 DESTINATION bin)
install(TARGETS isngsiv2rtpsbridgelib DESTINATION lib)
install(TARGETS isrtpsngsiv2bridgelib DESTINATION lib)

ament_package(
  CONFIG_EXTRAS "firos2-extras.cmake.in"
)

install(
  DIRECTORY cmake
  DESTINATION share/${PROJECT_NAME}
)
