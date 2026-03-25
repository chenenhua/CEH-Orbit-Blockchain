cmake_minimum_required(VERSION 3.16)

# =========================================================
# 项目名称
# =========================================================
project(CEH_Orbit LANGUAGES CXX)

# =========================================================
# C++ 标准配置
# =========================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =========================================================
# Qt 自动处理
# =========================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# =========================================================
# 工程路径
# =========================================================
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(ASSETS_DIR ${PROJECT_ROOT}/assets)

# =========================================================
# 应用信息
# =========================================================
set(APP_NAME "CEH_Orbit")
set(APP_OUTPUT_NAME "CEH-Orbit 区块链火种版")
set(APP_DISPLAY_NAME "CEH-Orbit 区块链火种版")
set(APP_AUTHOR "陈恩华")
set(APP_VERSION "1.0.0")
set(APP_BUNDLE_ID "com.wood.CEH.Orbit.Blockchain")

# =========================================================
# macOS 图标文件（必须是 .icns）
# =========================================================
set(MACOS_APP_ICON "${ASSETS_DIR}/img/icon/favicon.icns")

# =========================================================
# 自动选择 resources.qrc
# =========================================================
set(APP_RESOURCE_FILE "${PROJECT_ROOT}/resources.qrc")
if(NOT EXISTS "${APP_RESOURCE_FILE}")
    message(FATAL_ERROR "[QRC] 未找到 resources.qrc: ${APP_RESOURCE_FILE}")
else()
    message(STATUS "[QRC] 使用资源文件: ${APP_RESOURCE_FILE}")
endif()

# =========================================================
# 查找依赖
# =========================================================
find_package(Qt6 REQUIRED COMPONENTS
        Core
        Gui
        Widgets
        Charts
        PrintSupport
)

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBOQS REQUIRED liboqs)
find_package(OpenSSL REQUIRED)

# =========================================================
# Qt 标准项目设置
# =========================================================
qt_standard_project_setup()

# =========================================================
# macOS 图标资源打包
# =========================================================
set(APP_MACOS_RESOURCES "")
if(APPLE AND EXISTS "${MACOS_APP_ICON}")
    set(APP_MACOS_RESOURCES "${MACOS_APP_ICON}")
    set_source_files_properties("${MACOS_APP_ICON}" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
    )
    message(STATUS "[Mac图标] 已找到：${MACOS_APP_ICON}")
else()
    if(APPLE)
        message(WARNING "[Mac图标] 未找到：${MACOS_APP_ICON}")
    endif()
endif()

# =========================================================
# 创建可执行文件
# 必须用 MACOSX_BUNDLE，程序坞图标/名字
# =========================================================
qt_add_executable(${APP_NAME}
        MACOSX_BUNDLE
        WIN32
        main.cpp
        ${APP_RESOURCE_FILE}
        ${APP_MACOS_RESOURCES}
)

# =========================================================
# 头文件目录
# =========================================================
target_include_directories(${APP_NAME} PRIVATE
        ${PROJECT_ROOT}
        ${OpenCV_INCLUDE_DIRS}
        ${LIBOQS_INCLUDE_DIRS}
)

# =========================================================
# 链接目录
# =========================================================
target_link_directories(${APP_NAME} PRIVATE
        ${LIBOQS_LIBRARY_DIRS}
)

# =========================================================
# 链接库
# =========================================================
target_link_libraries(${APP_NAME} PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Charts
        Qt6::PrintSupport
        ${OpenCV_LIBS}
        ${LIBOQS_LIBRARIES}
        OpenSSL::Crypto
)

# =========================================================
# pkg-config 附带编译参数
# =========================================================
target_compile_options(${APP_NAME} PRIVATE
        ${LIBOQS_CFLAGS_OTHER}
)

# =========================================================
# 设置 bundle 属性
# =========================================================
set_target_properties(${APP_NAME} PROPERTIES
        OUTPUT_NAME "${APP_OUTPUT_NAME}"
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "${APP_DISPLAY_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${APP_BUNDLE_ID}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${APP_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}"
        MACOSX_BUNDLE_INFO_STRING "${APP_DISPLAY_NAME}"
        MACOSX_BUNDLE_COPYRIGHT "© ${APP_AUTHOR}"
)

# 图标存在时设置
if(APPLE AND EXISTS "${MACOS_APP_ICON}")
    set_target_properties(${APP_NAME} PROPERTIES
            MACOSX_BUNDLE_ICON_FILE "favicon.icns"
    )
endif()

# =========================================================
# macOS RPATH
# =========================================================
if(APPLE)
    set_target_properties(${APP_NAME} PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "@executable_path/../Frameworks"
    )
endif()

# =========================================================
# 安装规则
# =========================================================
install(TARGETS ${APP_NAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
)

# =========================================================
# Qt 部署脚本
# =========================================================
qt_generate_deploy_app_script(
        TARGET ${APP_NAME}
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})

# =========================================================
# Windows 控制台
# =========================================================
if(MSVC)
    set_target_properties(${APP_NAME} PROPERTIES
            LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

# =========================================================
# 构建信息输出
# =========================================================
message(STATUS "====================================================")
message(STATUS "[项目] 名称：${APP_NAME}")
message(STATUS "[项目] 显示名称：${APP_DISPLAY_NAME}")
message(STATUS "[项目] 版本：${APP_VERSION}")
message(STATUS "[QRC] 资源文件：${APP_RESOURCE_FILE}")
message(STATUS "[OpenCV] include：${OpenCV_INCLUDE_DIRS}")
message(STATUS "[OpenCV] libs：${OpenCV_LIBS}")
if(APPLE)
    message(STATUS "[macOS] 将生成 .app bundle")
endif()
message(STATUS "====================================================")