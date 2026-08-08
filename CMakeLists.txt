# 指定项目所需的最低 CMake 版本
cmake_minimum_required(VERSION 3.25)

# 强制指定 C 和 C++ 编译器，并将路径写入 CMake 缓存
set(CMAKE_C_COMPILER "/usr/bin/gcc" CACHE FILEPATH "C compiler" FORCE)
set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE FILEPATH "C++ compiler" FORCE)

# 使用 C++20 标准；必须支持该标准，并允许编译器扩展
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# 定义项目名称、版本以及使用的编程语言
project(pans VERSION 0.1.0 LANGUAGES CXX C)

# 引入 GNU 标准安装目录变量，如 CMAKE_INSTALL_BINDIR
include(GNUInstallDirs)

# 引入生成 CMake 包配置文件所需的辅助函数
include(CMakePackageConfigHelpers)

# 控制是否构建示例、测试和工具，默认全部开启
option(PANS_BUILD_EXAMPLES "Build examples" ON)
option(PANS_BUILD_TESTS    "Build tests"    ON)
option(PANS_BUILD_TOOLS    "Build tools"    ON)

# 对单配置生成器，在用户未指定构建类型时默认使用 Debug
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

# 限制 CMAKE_BUILD_TYPE 在常用构建类型中选择
set_property(
    CACHE CMAKE_BUILD_TYPE
    PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel
)

# 在配置阶段输出当前构建类型
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

# 获取当前系统的逻辑 CPU 核心数
cmake_host_system_information(
    RESULT CPU_NUMS
    QUERY NUMBER_OF_LOGICAL_CORES
)

# 输出逻辑 CPU 核心数
message(STATUS "CPU NUMS on current server is: ${CPU_NUMS}")

# 将默认并行任务数设置为逻辑核心数的 1.5 倍
math(EXPR JOBS "${CPU_NUMS} + ${CPU_NUMS} / 2")

# 把并行任务数写入缓存，供 CMake 构建阶段使用
set(
    CMAKE_BUILD_PARALLEL_LEVEL
    ${JOBS}
    CACHE STRING
    "并行编译线程数"
)

# 输出最终采用的默认并行任务数
message(
    STATUS
    "Compiler default work jobs: ${CMAKE_BUILD_PARALLEL_LEVEL}"
)

# 把编译选项集中放到 cmake/pansOptions.cmake 中
include(cmake/pansOptions.cmake)

add_subdirectory(pans)
if(PANS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
if(PANS_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

