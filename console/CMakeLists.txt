cmake_minimum_required (VERSION 3.8)

set(PROJECT_NAME "console")

add_library(${PROJECT_NAME} STATIC Console.cpp Console.h Console_io.cpp Console_io.h ProFont6x11.h
 T2Input.cpp T2Input.h)

target_include_directories(${PROJECT_NAME} PUBLIC "." "../main" "../IPtoStream")

target_link_libraries(${PROJECT_NAME} mreapi main IPtoStream)
