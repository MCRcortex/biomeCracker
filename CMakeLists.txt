cmake_minimum_required(VERSION 3.10)
project(crackinator)

set(CMAKE_CXX_STANDARD 14)
add_executable(crackinator generationByCubitect/generator.cpp generationByCubitect/layers.cpp generationByCubitect/finders.cpp main.cpp)