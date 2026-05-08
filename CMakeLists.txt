# 设置cmake最低版本
cmake_minimum_required(VERSION 3.16)

# 项目名称
project(MissionPlannerApp)

# =================================================================================
# Qt C++ 相关配置
# =================================================================================

# 设置自动MOC、UIC和RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

#根据运行平台 设置QT安装路径
if(WIN32)
    set(QT_DIR "C:/Qt/Qt5.12.12/5.12.12/msvc2017_64")
else ()
    set(QT_DIR "/usr/local/bin/Qt5.9.0/bin/gcc_64")
endif ()
# 指定Qt 安装路径
set(CMAKE_PREFIX_PATH ${QT_DIR})

# 明确启用 C++ 支持
#enable_language(CXX)
# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
# 强制要求使用指定的C++标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 设置编译文件默认为：UTF-8 编码
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")

# 设置输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin_windows)

# =================================================================================
# 项目配置
# =================================================================================

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

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Gui Qt5::Widgets)
