cmake_minimum_required(VERSION 3.10)
project(cpp_transport_catalogue)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_PREFIX_PATH c:/dev/pro3)
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto tc_base.proto tc_visual.proto tc_routing.proto)

set(TC_FILES main.cpp geo.h transport_catalogue.cpp transport_catalogue.h domain.h domain.cpp geo.cpp json.cpp json.h json_reader.cpp json_reader.h request_handler.cpp request_handler.h svg.cpp svg.h map_renderer.cpp map_renderer.h json_builder.cpp json_builder.h graph.h ranges.h router.h transport_router.cpp transport_router.h serialization.cpp serialization.h)

add_executable(transport_catalogue  ${PROTO_SRCS} ${PROTO_HDRS} ${TC_FILES} ${Protobuf_PREFIX_PATH})

target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue ${Protobuf_LIBRARY_DEBUG} Threads::Threads)

message(STATUS "<<<***TC Config: ${CONFIG}, Libraries: ${Protobuf_LIBRARY_DEBUG} ***>>>")