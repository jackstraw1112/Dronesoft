cmake_minimum_required(VERSION 3.16)
project(MissionPlannerApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(CMAKE_VERSION VERSION_LESS "3.10")
    set(CMAKE_AUTOMOC_RELAXED_MODE TRUE)
endif()

add_compile_options("/utf-8")

set(CMAKE_PREFIX_PATH "C:/Qt/Qt5.12.12/5.12.12/msvc2017_64")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../bin_windows)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source)

set(SOURCES
    source/main.cpp
    source/MissionPlanner.cpp
    source/TaskListWidget.cpp
    source/RightSidePanel.cpp
    source/WindowStyle.cpp
    source/AddOrEditTaskplan.cpp
    source/ForceRequirementPanel.cpp
    source/ParameterLoading.cpp
    source/RoutePlanning.cpp
    source/TaskAllocationPanel.cpp
)

set(HEADERS
    source/MissionPlanner.h
    source/TaskListWidget.h
    source/RightSidePanel.h
    source/StructData.h
    source/WindowStyle.h
    source/AddOrEditTaskplan.h
    source/ForceRequirementPanel.h
    source/ParameterLoading.h
    source/RoutePlanning.h
    source/TaskAllocationPanel.h
)

set(UI_FILES
    source/MissionPlanner.ui
    source/AddOrEditTaskplan.ui
    source/ForceRequirementPanel.ui
    source/ParameterLoading.ui
    source/TaskAllocationPanel.ui
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
)

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Gui Qt5::Widgets)
