cmake_minimum_required(VERSION 3.20)
project(franka-streaming LANGUAGES CXX)

# Standalone build: does not configure or modify the old proxy targets.
find_package(Franka CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ruckig 0.15.3 EXACT CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_library(streaming_dependencies INTERFACE)
target_compile_features(streaming_dependencies INTERFACE cxx_std_17)
target_include_directories(streaming_dependencies INTERFACE src)
target_link_libraries(streaming_dependencies INTERFACE
    asio::asio nlohmann_json::nlohmann_json ruckig::ruckig Threads::Threads)
if(WIN32)
    target_compile_definitions(streaming_dependencies INTERFACE _WIN32_WINNT=0x0A00 NOMINMAX)
endif()
if(MSVC)
    target_compile_options(streaming_dependencies INTERFACE /W4)
else()
    target_compile_options(streaming_dependencies INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_executable(franka_stream_server src/server.cpp)
target_link_libraries(franka_stream_server PRIVATE streaming_dependencies Franka::Franka)

include(CTest)
if(BUILD_TESTING)
    add_executable(streaming_test tests/control_test.cpp)
    target_link_libraries(streaming_test PRIVATE streaming_dependencies)
    add_test(NAME streaming_control_network_offline COMMAND streaming_test)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME streaming_client_offline
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/client_test.py)
    set_tests_properties(streaming_control_network_offline streaming_client_offline
        PROPERTIES TIMEOUT 15)
endif()
