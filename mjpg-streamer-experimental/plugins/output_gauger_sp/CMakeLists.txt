enable_language(CXX)

#find_package(SQLite::SQLite3)

MJPG_STREAMER_PLUGIN_OPTION(output_gauger_sp "Gauger output plugin for signal poinit")

MJPG_STREAMER_PLUGIN_COMPILE(output_gauger_sp
#    cal_control.cpp
#    calculation.cpp
#    cir_queue.cpp
    cmd.cpp
    output_gauger.cpp
    log.c
#    thread.cpp
#    CppSQLite3.cpp
)

target_link_directories(output_gauger_sp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/x64)

target_link_libraries(output_gauger_sp gauger sqlite3)
