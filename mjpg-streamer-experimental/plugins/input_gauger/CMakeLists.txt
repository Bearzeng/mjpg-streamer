
set(PROJECT_NAME input_gauger)


find_package(OpenCV COMPONENTS core imgproc highgui videoio)

MJPG_STREAMER_PLUGIN_OPTION(${PROJECT_NAME} "HikRobot input plugin"
                            ONLYIF OpenCV_FOUND )

if (PLUGIN_INPUT_GAUGER)
    enable_language(CXX)
    include_directories(${OpenCV_INCLUDE_DIRS})

    include_directories(sdk/x86_64/include)
    include_directories(gauger)

    MJPG_STREAMER_PLUGIN_COMPILE(${PROJECT_NAME}
        input_gauger.cpp
        hik_capture.cpp
        log.c
    )

    target_link_directories(${PROJECT_NAME} PRIVATE
        sdk/x86_64/lib/64
        ${CMAKE_CURRENT_SOURCE_DIR}/gauger/x64
    )

    target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} MvCameraControl gauger sqlite3)
       
    
endif()


