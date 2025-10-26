# CMakeLists.txt for ZYGL2 Project
# 资源管理系统 CMake 配置文件

cmake_minimum_required(VERSION 3.10)
project(zygl2 VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 包含目录
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 编译选项
option(ENABLE_HTTPS "Enable HTTPS support (requires OpenSSL)" OFF)
option(ENABLE_ZLIB "Enable zlib compression support" OFF)
option(BUILD_TESTS "Build test programs" ON)
option(BUILD_MAIN "Build main program (requires src/main.cpp)" OFF)

# 显示配置信息
message(STATUS "==================================")
message(STATUS "ZYGL2 Project Configuration")
message(STATUS "==================================")
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Enable HTTPS: ${ENABLE_HTTPS}")
message(STATUS "Enable ZLIB: ${ENABLE_ZLIB}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Build main: ${BUILD_MAIN}")
message(STATUS "==================================")

# 查找依赖库
find_package(Threads REQUIRED)

# OpenSSL支持（可选）
if(ENABLE_HTTPS)
    find_package(OpenSSL REQUIRED)
    if(OPENSSL_FOUND)
        message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")
        add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
        include_directories(${OPENSSL_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "OpenSSL not found but ENABLE_HTTPS is ON")
    endif()
endif()

# zlib支持（可选）
if(ENABLE_ZLIB)
    find_package(ZLIB REQUIRED)
    if(ZLIB_FOUND)
        message(STATUS "ZLIB found: ${ZLIB_VERSION_STRING}")
        add_definitions(-DCPPHTTPLIB_ZLIB_SUPPORT)
        include_directories(${ZLIB_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "ZLIB not found but ENABLE_ZLIB is ON")
    endif()
endif()

# 源文件分组（用于IDE）
file(GLOB_RECURSE DOMAIN_HEADERS "src/domain/*.h")
file(GLOB_RECURSE INFRASTRUCTURE_HEADERS "src/infrastructure/*.h")
file(GLOB_RECURSE APPLICATION_HEADERS "src/application/*.h")
file(GLOB_RECURSE INTERFACES_HEADERS "src/interfaces/*.h")

# 创建源文件组（用于IDE项目浏览）
source_group("Domain Layer" FILES ${DOMAIN_HEADERS})
source_group("Infrastructure Layer" FILES ${INFRASTRUCTURE_HEADERS})
source_group("Application Layer" FILES ${APPLICATION_HEADERS})
source_group("Interfaces Layer" FILES ${INTERFACES_HEADERS})

# 辅助函数：链接通用库
function(link_common_libraries target)
    target_link_libraries(${target} PRIVATE Threads::Threads)
    
    if(ENABLE_HTTPS AND OPENSSL_FOUND)
        target_link_libraries(${target} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
    
    if(ENABLE_ZLIB AND ZLIB_FOUND)
        target_link_libraries(${target} PRIVATE ZLIB::ZLIB)
    endif()
endfunction()

# 构建测试程序
if(BUILD_TESTS)
    # 依赖库测试
    add_executable(test_dependencies test_dependencies.cpp)
    link_common_libraries(test_dependencies)
    
    # 领域层测试
    add_executable(test_domain test_domain.cpp)
    link_common_libraries(test_domain)
    
    message(STATUS "Test programs will be built")
endif()

# 构建主程序
if(BUILD_MAIN)
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/main.cpp")
        add_executable(zygl2 src/main.cpp)
        link_common_libraries(zygl2)
        message(STATUS "Main program will be built")
    else()
        message(WARNING "src/main.cpp not found, skipping main program build")
    endif()
endif()

# 安装规则
if(BUILD_MAIN AND EXISTS "${CMAKE_SOURCE_DIR}/src/main.cpp")
    install(TARGETS zygl2
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
endif()

# 安装头文件（如果需要作为库使用）
install(DIRECTORY src/
        DESTINATION include/zygl2
        FILES_MATCHING PATTERN "*.h")

# 安装第三方库头文件
install(FILES third_party/httplib.h third_party/json.hpp
        DESTINATION include/zygl2/third_party)

# 安装文档
install(FILES README.md 编译指南.md 项目目录结构.md
        DESTINATION share/doc/zygl2)

# CTest支持
if(BUILD_TESTS)
    enable_testing()
    
    add_test(NAME test_dependencies
             COMMAND test_dependencies
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    
    add_test(NAME test_domain
             COMMAND test_domain
             WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# CPack配置（打包）
set(CPACK_PACKAGE_NAME "zygl2")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "基于DDD的资源管理系统")
set(CPACK_PACKAGE_VENDOR "ZYGL2 Team")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_GENERATOR "TGZ;ZIP")

include(CPack)

# 自定义目标

# 格式化代码
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i 
            ${CMAKE_SOURCE_DIR}/test_dependencies.cpp
            ${CMAKE_SOURCE_DIR}/test_domain.cpp
        COMMENT "Formatting code with clang-format"
    )
endif()

# 静态分析
find_program(CPPCHECK cppcheck)
if(CPPCHECK)
    add_custom_target(check
        COMMAND ${CPPCHECK} --enable=all --std=c++17 
            --suppress=missingIncludeSystem
            ${CMAKE_SOURCE_DIR}/src/
        COMMENT "Running cppcheck static analysis"
    )
endif()

# 统计信息
add_custom_target(stats
    COMMAND echo "项目统计信息："
    COMMAND echo "  源代码文件: `find ${CMAKE_SOURCE_DIR}/src -name '*.h' | wc -l` 个"
    COMMAND echo "  文档文件: `ls ${CMAKE_SOURCE_DIR}/docs/*.md 2>/dev/null | wc -l` 个"
    COMMAND echo "  第三方库: 2 个"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Displaying project statistics"
)

# 显示构建信息
message(STATUS "==================================")
message(STATUS "Build configuration completed")
message(STATUS "==================================")
message(STATUS "To build the project:")
message(STATUS "  cmake --build .")
message(STATUS "  or: make")
message(STATUS "")
message(STATUS "To run tests:")
message(STATUS "  ctest")
message(STATUS "  or: make test")
message(STATUS "")
message(STATUS "To install:")
message(STATUS "  sudo make install")
message(STATUS "==================================")

