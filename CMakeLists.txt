cmake_minimum_required(VERSION 3.16)

project(simple_video_speed_changer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt modules
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(PROJECT_SOURCES
    main.cpp
    main_window.hpp
    video_speed_changer_widget.h
    video_speed_changer_widget.cpp
)

set(EXE_NAME
    video_speed_changer
)

qt_add_executable(${EXE_NAME}
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

qt_add_resources(${EXE_NAME} "resources"
        PREFIX "/"
        FILES assets/icon.png
)

target_link_libraries(${EXE_NAME}
    PRIVATE Qt6::Core Qt6::Widgets
)

qt_finalize_executable(${EXE_NAME})
