cmake_minimum_required(VERSION 3.23)
project(bch-tree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(BehaviorTree REQUIRED)
find_package(spdlog REQUIRED)
find_package(cxxopts REQUIRED)

include_directories("$ENV{EPICS_BASE}/include" "$ENV{EPICS_BASE}/include/os/Linux")
link_directories("$ENV{EPICS_BASE}/lib/linux-x86_64")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  include_directories("$ENV{EPICS_BASE}/include/compiler/gcc")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  include_directories("$ENV{EPICS_BASE}/include/compiler/clang")
else()
  message(FATAL_ERROR "Unsupported compiler for EPICS headers: ${CMAKE_CXX_COMPILER_ID}")
endif()

add_library(bchtree
    src/bt_runner.cpp
    src/logger.cpp
    src/epics/ca/ca_pv.cpp
    src/epics/ca/ca_context_manager.cpp
    src/epics/ca/ca_pv_manager.cpp
)
target_include_directories(bchtree PUBLIC include)
target_link_libraries(bchtree PUBLIC
    BT::behaviortree_cpp
    spdlog::spdlog
    ca
    Com
)

add_executable(bch-tree-cli src/main.cpp)
target_link_libraries(bch-tree-cli PRIVATE bchtree cxxopts::cxxopts spdlog::spdlog)

install(TARGETS bch-tree-cli RUNTIME DESTINATION bin)
