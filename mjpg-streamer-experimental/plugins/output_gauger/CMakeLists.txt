enable_language(CXX)

#find_package(SQLite::SQLite3)

MJPG_STREAMER_PLUGIN_OPTION(output_gauger "Gauger output plugin")

MJPG_STREAMER_PLUGIN_COMPILE(output_gauger
    cal_control.cpp
    calculation.cpp
    cir_queue.cpp
    cmd.cpp
    output_gauger.cpp
    thread.cpp
    CppSQLite3.cpp
)

target_link_directories(output_gauger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/x64)

target_link_libraries(output_gauger sqlite3 gauger)
