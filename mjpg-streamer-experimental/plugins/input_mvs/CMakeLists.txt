
# TODO: which components do I need?
# To fix the error: "undefined symbol: _ZN2cv12VideoCaptureC1Ev"
find_package(OpenCV COMPONENTS core imgproc highgui videoio)

MJPG_STREAMER_PLUGIN_OPTION(input_mvs "HikRobot input plugin"
                            ONLYIF OpenCV_FOUND )

if (PLUGIN_INPUT_MVS)
    enable_language(CXX)
    include_directories(${OpenCV_INCLUDE_DIRS})

    include_directories(sdk/x86_64/include)

    MJPG_STREAMER_PLUGIN_COMPILE(input_mvs
        input_mvs.cpp
        hik_capture.cpp
    )

    target_link_directories(input_mvs PRIVATE sdk/x86_64/lib/64)

    target_link_libraries(input_mvs ${OpenCV_LIBS} MvCameraControl)
       
    
endif()


