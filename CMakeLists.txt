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


target_compile_options(GraphDebugger PUBLIC -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wconversion -Wno-unused-parameter -static-libstdc++)
target_compile_features(GraphDebugger PUBLIC cxx_std_17)

add_subdirectory(glfw)
target_link_libraries(GraphDebugger PUBLIC glfw)

target_include_directories(GraphDebugger PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}
    glfw/include
    include
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
