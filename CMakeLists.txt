cmake_minimum_required(VERSION 3.16)

project(my_project VERSION 0.1 LANGUAGES CXX)

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
)

set(EXE_NAME
    my_executable
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
