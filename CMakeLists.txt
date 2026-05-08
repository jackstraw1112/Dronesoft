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

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Sql)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source/01-TaskPlan)

# 自动收集源文件和头文件，减少手动维护
# 注意：GLOB 不适用于需要精确控制文件顺序的场景
file(GLOB SOURCES CONFIGURE_DEPENDS
        "*.cpp"
        "source/*.cpp"
        "source/01-TaskPlan/*.cpp"
)

file(GLOB HEADERS CONFIGURE_DEPENDS
        "*.h"
        "source/*.h"
        "source/01-TaskPlan/*.h"
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Sql)

# 统一主题样式：构建后复制到可执行目录下的 theme/ 目录
set(UNIFIED_THEME_QSS "${CMAKE_CURRENT_SOURCE_DIR}/source/theme/mission_planner_theme.qss")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/theme"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${UNIFIED_THEME_QSS}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/theme/mission_planner_theme.qss"
)

# =================================================================================
# 项目公共配置
# =================================================================================

# 设置输出目录
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set_target_properties(${PROJECT_NAME} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin_linux_x86_64"        # app输出目录
                ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin_linux_x86_64/lib"    # so输出目录
        )
    elseif (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
        set_target_properties(${PROJECT_NAME} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin_linux_x86_64"        # app输出目录
                ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin_linux_x86_64/lib"    # so输出目录
        )
    elseif (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "ARM64")
        set_target_properties(${PROJECT_NAME} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin_linux_aarch64"        # app输出目录
                ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin_linux_aarch64/lib"    # so输出目录
        )
    else ()
        set_target_properties(${PROJECT_NAME} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin_linux_loongarch64"        # app输出目录
                ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin_linux_loongarch64/lib"    # so输出目录
        )
    endif ()
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # Windows 下直接设置目标属性，确保对已创建目标生效
    set_target_properties(${PROJECT_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin_windows"          # exe/dll 输出目录
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin_windows/lib"      # .lib 输出目录
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin_windows/plugins"  # 其他库输出目录
    )

    # 多配置生成器（如 VS）下，避免产物落到 Debug/Release 子目录
    set_target_properties(${PROJECT_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin_windows"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin_windows"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin_windows"
            RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/bin_windows"
            ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin_windows/lib"
            ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin_windows/lib"
            ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin_windows/lib"
            ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/bin_windows/lib"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin_windows/plugins"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin_windows/plugins"
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin_windows/plugins"
            LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/bin_windows/plugins"
    )
endif ()

# 设置数据目录变量：OUTPUT_DIR
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OUTPUT_DIR "bin_linux_x64")
    elseif (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
        set(OUTPUT_DIR "bin_linux_x64")
    elseif (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "ARM64")
        set(OUTPUT_DIR "bin_linux_aarch64")
    else ()
        set(OUTPUT_DIR "bin_linux_loongarch64")
    endif ()
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(OUTPUT_DIR "bin_windows")
endif ()