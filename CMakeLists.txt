cmake_minimum_required(VERSION 3.0.0)
project(GraphDebugger VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()


add_library(GraphDebugger STATIC 
    src/glad.c
    src/Shader.cpp
    src/ShaderProgram.cpp
    src/VertexArray.cpp
    src/Window.cpp
    src/GraphTab.cpp
    src/Graph.cpp
    src/utils.cpp
    src/GraphArrangement.cpp
    src/hardcoded_texture_atlas.cpp
    src/InputHandler.cpp
)
  
if(MSVC)
    target_compile_options(GraphDebugger PUBLIC /W4)
else()
    target_compile_options(GraphDebugger PUBLIC -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wconversion -Wno-unused-parameter)
endif()

target_compile_features(GraphDebugger PUBLIC cxx_std_17)

add_subdirectory(glfw)
target_link_libraries(GraphDebugger PUBLIC glfw)
target_include_directories(GraphDebugger PRIVATE 
    include
)

