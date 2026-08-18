set(TARGET_NAME "main")

set(SOURCES main.cpp thread.cpp thread.h libco.cpp libco.h)

add_exec_vxp(${TARGET_NAME} 
    SRCS ${SOURCES}
    RESOURCES resources
    APP_NAME "${APP_NAME}"
    APPID "${APPID}"
    BACKGROUND ${BACKGROUND}
    API "${API}"
    RAM "${RAM}"
)

target_include_directories(${TARGET_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${TARGET_NAME} mreapi console bt_opp IPtoStream)
