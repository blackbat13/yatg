cmake_minimum_required(VERSION 3.21)
project(yatg)

set(CMAKE_CXX_STANDARD 14)

add_library(yatg STATIC include/yatg/yatg.cpp include/yatg/yatg.hpp)

install(TARGETS yatg DESTINATION lib)
install(FILES yatg.hpp DESTINATION include)