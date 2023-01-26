cmake_minimum_required(VERSION 3.23)
project(cpp_transport_catalogue)

set(CMAKE_CXX_STANDARD 17)

add_executable(transport_catalogue main.cpp geo.h transport_catalogue.cpp transport_catalogue.h domain.h domain.cpp geo.cpp json.cpp json.h json_reader.cpp json_reader.h request_handler.cpp request_handler.h svg.cpp svg.h map_renderer.cpp map_renderer.h)
