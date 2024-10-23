cmake_minimum_required(VERSION 3.8)
project(my_sync VERSION 0.1.0 LANGUAGES C CXX)

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(DEBUG_INFO "enable debug messages" ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# 导出 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(INSTALL_DIR ${CMAKE_SOURCE_DIR}/install)

# find dependencies
find_package(roscpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)


# # file(GLOB SRC src/*.cpp)
# file(GLOB_RECURSE SRC src/*.cpp)

# 同步算法库
add_library(approximate_algo STATIC algo/approximate_time_algo.cpp)
target_link_libraries(approximate_algo PRIVATE
    ${roscpp_LIBRARIES}
)
target_include_directories(approximate_algo PRIVATE
    .
    ${roscpp_INCLUDE_DIRS}
)

# demo 程序
add_executable(${PROJECT_NAME} main.cpp sync/channel_manager.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${roscpp_LIBRARIES}
    approximate_algo
)
target_include_directories(${PROJECT_NAME} PRIVATE
    .
    ${roscpp_INCLUDE_DIRS}
)

# 设置 c++ 标准
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_compile_features(approximate_algo PUBLIC cxx_std_17)

# target_compile_options(${PROJECT_NAME}
#                        PUBLIC -W -Wall -Wextra -pedantic -pedantic-errors)

if (DEBUG_INFO)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DDEBUGINFO)
    target_compile_definitions(approximate_algo PRIVATE -DDEBUGINFO)
endif()

# target_compile_definitions(${PROJECT_NAME} PRIVATE -DPI=3.1415926)

# install(TARGETS ${PROJECT_NAME}
#         LIBRARY DESTINATION lib  # 动态库安装路径
#         ARCHIVE DESTINATION lib  # 静态库安装路径
#         RUNTIME DESTINATION bin  # 可执行文件安装路径
#         PUBLIC_HEADER DESTINATION include  # 头文件安装路径
# )

# install(FILES abc DESTINATION ${INSTALL_DIR})
# install(DIRECTORY ${CMAKE_SOURCE_DIR}/third_party/airos-sdk/conf ${CMAKE_SOURCE_DIR}/third_party/airos-sdk/lib DESTINATION ${INSTALL_DIR}/airos)