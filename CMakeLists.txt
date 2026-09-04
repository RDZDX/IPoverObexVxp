cmake_minimum_required (VERSION 3.10)

project ("IPoverObexVxp")

set(APP_NAME "IPoverObexVxp")
set(DEVELOPER_NAME "Ximik_Boda")
set(RAM "1500") #Ram requirement (3250 in kb)
set(IMSI "1234567890" CACHE STRING "Imsi of first SIM-card")
set(API "Audio File SIM_card Sec SysStorage SysFile ProMng BT") #Api for app,
set(APPID "-1") 
set(CERTID "1" CACHE STRING "") 
set(CERT "none" CACHE STRING "")
option(BACKGROUND "App can work background" OFF)

set(MRE_SDK "$ENV{MRE_SDK}" CACHE STRING "MRE SDK dir")
set(TinyMRESDK "$ENV{TinyMRESDK}" CACHE STRING "TinyMRESDK dir")

if(BACKGROUND)
    add_definitions(-DSUPPORT_BG)
endif()

include(cmake/MreUtils.cmake)

add_subdirectory(resources)
add_subdirectory(mreapi)
add_subdirectory(run)
add_subdirectory(bt_opp)
add_subdirectory(console)
add_subdirectory(IPtoStream)

add_subdirectory(main)

#Full api list: Audio Camera Call TCP File HTTP Sensor SIM_card Record SMS_person SMS_SP BitStream Contact LBS MMS ProMng SMSMng Video XML Sec SysStorage Payment BT PUSH UDP SysFile
