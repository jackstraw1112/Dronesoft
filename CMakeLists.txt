cmake_minimum_required(VERSION 3.16)
project(MissionPlannerApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source)

set(SOURCES
    source/MissionPlanner.cpp
    source/TaskListWidget.cpp
    source/RightSidePanel.cpp
    source/WindowStyle.cpp
)

set(HEADERS
    source/MissionPlanner.h
    source/TaskListWidget.h
    source/RightSidePanel.h
    source/StructData.h
    source/WindowStyle.h
)

set(UI_FILES
    source/MissionPlanner.ui
    source/TaskListWidget.ui
    source/RightSidePanel.ui
)

add_executable(${PROJECT_NAME}
    source/main.cpp
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
)

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Gui Qt5::Widgets)
