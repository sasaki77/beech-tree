from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class BeechTree(ConanFile):
    name = "beech-tree"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"
    requires = (
        "behaviortree.cpp/4.7.2",
        "spdlog/1.13.0",
        "cxxopts/3.1.1",
    )
    generators = "CMakeToolchain", "CMakeDeps"

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
