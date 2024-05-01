cmake_minimum_required(VERSION 3.25.2 FATAL_ERROR)

project(cachio)

set(CMAKE_CXX_STANDARD 17)
set(BOOST_INCLUDE_LIBRARIES filesystem system date_time thread graph asio log)
set(BOOST_ENABLE_CMAKE ON)
set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.81.0 REQUIRED COMPONENTS filesystem thread log)

set_target_properties(Boost::headers PROPERTIES IMPORTED_GLOBAL TRUE)
set_target_properties(Boost::log PROPERTIES IMPORTED_GLOBAL TRUE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCES ./src/main.cpp)

add_executable(cachio ${SOURCES})
