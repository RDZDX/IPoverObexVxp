set(PROJECT_NAME "IPtoStream")

add_library(${PROJECT_NAME} STATIC 
    IPtoStream.h IPtoStream.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC ".")

target_link_libraries(${PROJECT_NAME} mreapi bt_opp)