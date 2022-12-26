cmake_minimum_required(VERSION 3.23)
project(cpp_transport_catalogue)

set(CMAKE_CXX_STANDARD 17)

add_executable(transport_catalogue main.cpp geo.h input_reader.cpp input_reader.h stat_reader.cpp stat_reader.h transport_catalogue.cpp transport_catalogue.h)
