#  INTERFACE 接口库，普通 add_library 会编译源码、生成 .a/.so 文件。而 INTERFACE 库没有任何源码，也不产生任何二进制文
#  件——它只是一个"标签"或"接口"，专门用来往上传配置（编译选项、宏定义、链接选项），然后把这些配置传
#  给链接它的人。所以叫接口库。
add_library(pans_options INTERFACE) 

# 为链接 pans_options 的目标添加编译选项
target_compile_options(pans_options INTERFACE
    # 仅当 C++ 编译器为 GCC 或 Clang 时启用以下选项
    # cmake的生成器表达式：$<条件:值>

    #   在 Linux + g++ 下展开过程是：
    #   $<CXX_COMPILER_ID:GNU>   → 1（是 gcc）
    #   $<CXX_COMPILER_ID:Clang> → 0
    #   $<OR:1,0>                → 1
    #   $<1: -Wall -Wextra ...>  → -Wall -Wextra -Wpedantic -fno-strict-aliasing
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        -Wall                 # 启用常用警告
        -Wextra               # 启用额外警告，比 -Wall 更严格
        -Wpedantic            # 严格检查不符合语言标准的代码
        -fno-strict-aliasing  # 禁用严格别名优化
    >
)

target_compile_options(pans_options INTERFACE
    $<$<AND:$<PLATFORM_ID:Linux>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:
        -fPIC          # 生成位置无关代码。共享库（.so）的代码要能被加载到内存任意地址，必须这么编译。
    >
)

# 链接选项
target_link_options(pans_options INTERFACE
    $<$<AND:$<PLATFORM_ID:Linux>,$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>>:
        -rdynamic      # 让可执行文件的符号表对动态链接器可见。效果是：程序崩溃或打印栈回溯时显示函数名而不是地址
    >
)

target_compile_definitions(pans_options INTERFACE
    $<$<CONFIG:Debug>:PANS_DEBUG> # 定义宏 PANS_DEBUG
)

target_compile_options(pans_options INTERFACE
    $<$<CONFIG:Debug>:-O0>  # 不优化，不进行指令编排
    $<$<CONFIG:Debug>:-g3>  # 输入更多编译信息
    $<$<CONFIG:Debug>:-ggdb> # 调试信息按 gdb 格式生成
)

# ===== Release/RelWithDebInfo 共用选项 =====
target_compile_options(pans_options INTERFACE
    $<$<CONFIG:Release>:-DNDEBUG> # 定义了 NDEBUG 后 assert() 失效
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Release>:-fno-omit-frame-pointer> # 保留栈帧指针。优化时编译器通常省掉它，但没了它栈回溯和性能剖析工具就抓
    # 不到调用链。代价是极小的性能损失，换可靠的调试/分析能力

    $<$<CONFIG:RelWithDebInfo>:-DNDEBUG>
    $<$<CONFIG:RelWithDebInfo>:-O2>
    $<$<CONFIG:RelWithDebInfo>:-g> # 带调试信息
    $<$<CONFIG:RelWithDebInfo>:-fno-omit-frame-pointer>
)

# --coverage 让编译器给代码插桩（配合 gcov），跑完测试能统计"哪行代码被执行过"，生成覆盖率报告。
option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)
if(ENABLE_COVERAGE)
    target_compile_options(pans_options INTERFACE
        $<$<CONFIG:Debug>:--coverage>
    )
    target_link_options(pans_options INTERFACE
        $<$<CONFIG:Debug>:--coverage>
    )
endif(ENABLE_COVERAGE)