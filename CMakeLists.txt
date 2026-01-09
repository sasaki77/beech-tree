cmake_minimum_required(VERSION 3.23)
project(bt_epics_runner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(BehaviorTree REQUIRED)
find_package(spdlog REQUIRED)
find_package(cxxopts REQUIRED)

add_executable(beech-tree src/main.cpp)
target_link_libraries(beech-tree PRIVATE BT::behaviortree_cpp cxxopts::cxxopts spdlog::spdlog)

install(TARGETS beech-tree RUNTIME DESTINATION bin)
