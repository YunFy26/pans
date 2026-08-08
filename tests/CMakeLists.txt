# GLOB 是 通配符匹配
# 把 tests/ 目录下所有匹配 *.cpp 的文件路径收集起来，存到 PANS_TEST_SOURCES 这个变量里
# CONFIGURE_DEPENDS 是 CMake 3.12+ 的选项：告诉 CMake 每次构建前自动检查这个目录里的文件有没有增
# 删。没有它的话，以后新增一个测试 .cpp 文件，必须手动重新运行 cmake 配置才会被收进来
file(GLOB PANS_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
)

# 文件排序
list(SORT PANS_TEST_SOURCES)

# 找线程库
find_package(Threads REQUIRED)

# 设置通过cmake构建好的可执行程序的输出目录
set(PANS_TEST_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/tests")

foreach(TEST_SOURCE IN LISTS PANS_TEST_SOURCES)
    # STATUS 是 message() 的另一种级别，表示"普通提示信息"
    message(STATUS "===============${TEST_SOURCE}")   

    # NAME_WE: Name Without Extension
    get_filename_component(TEST_NAME "${TEST_SOURCE}" NAME_WE)
    message(STATUS ">>>>>>>>>>>>>>>${TEST_NAME}")    

    # TARGET 是 if() 的一种判断条件，意思是"名字叫这个的目标是否已经存在"。
    # 这里检查 TEST_NAME
    if(TARGET "${TEST_NAME}")
        message(FATAL_ERROR "Cannot create test target '${TEST_NAME}': a target with that name already exists.")
    endif(TARGET "${TEST_NAME}")
    
    # 可执行文件名/源文件
    add_executable("${TEST_NAME}" "${TEST_SOURCE}")

    # 链接库
    # pans 的 PUBLIC 依赖会让测试同时继承头文件路径和 pans_options
    target_link_libraries("${TEST_NAME}" PRIVATE pans Threads::Threads)

    # set_target_properties(目标 PROPERTIES 属性名 属性值 ...)：CMake 用来给已创建的目标设置属性的命令
    # PROPERTIES 后面跟成对的"属性名 + 值"
    # RUNTIME_OUTPUT_DIRECTORY 可执行文件生成后放哪
    set_target_properties("${TEST_NAME}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PANS_TEST_OUTPUT_DIRECTORY}"
    )

    # # 给多配置构建生成器（Visual Studio、Xcode、Ninja Multi-Config）单独设置每个配置的输出目录
    # # 多配置生成器默认会追加 Debug/Release 子目录，这里保持输出路径一致
    # # CMAKE_CONFIGURATION_TYPES 就是多配置生成器下所有可用配置名的列表：Debug;Release;MinSizeRel;RelWithDebInfo
    # foreach(CONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
    #     # 把配置名转为大写
    #     string(TOUPPER "${CONFIG}" CONFIG_UPPER)
    #     set_target_properties("${TEST_NAME}" PROPERTIES
    #         "RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER}" "${PANS_TEST_OUTPUT_DIRECTORY}"
    #     )
    # endforeach(CONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)

    # $<CONFIG> 在单配置生成器下展开成 CMAKE_BUILD_TYPE（Debug/Release...）
    set_target_properties("${TEST_NAME}" PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PANS_TEST_OUTPUT_DIRECTORY}/$<CONFIG>"
    )
    
    # 把刚刚构建的可执行程序注册成CTEST测试，让ctest命令可以运行它
    # 运行 ctest 时，它会找到所有注册的测试，逐个执行对应的可执行文件
    # NAME 是测试在 CTest 里的显示名，COMMAND 是要运行的程序
    add_test(NAME "${TEST_NAME}" COMMAND "${TEST_NAME}")

endforeach(TEST_SOURCE IN LISTS PANS_TEST_SOURCES)
