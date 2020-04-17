cmake_minimum_required(VERSION 3.15)
project(untitled)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fPIE -pie -g")

set(Boost_USE_MULTITHREADED ON)

set(CMAKE_CXX_STANDARD_LIBRARIES -lboost_locale)

find_package(Boost REQUIRED COMPONENTS filesystem locale system)

find_package(LibArchive REQUIRED)
find_package(Threads REQUIRED)

add_executable(counting_words1 main.cpp)

target_link_libraries(counting_words1 Threads::Threads ${LibArchive_LIBRARIES})