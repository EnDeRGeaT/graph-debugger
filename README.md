Small Graph drawing library which makes debugging graph problems easier!
## Prerequisites
- CMake 3.0+
- Git 1.8.2+

## Build
### For GCC/Clang
If you use Linux, you need to install dependencies for GLFW. \
See: [GLFW Installing Dependencies](https://www.glfw.org/docs/latest/compile.html#compile_deps) 

Copy and paste the following lines in the shell.
```shell
git clone --recurse-submodules https://github.com/EnDeRGeaT/graph-debugger
cd graph-debugger
mkdir build
cd build
cmake -G "Unix Makefiles" ..
make

```
After build has been completed, you will have two libraries you need. \
1. libGraphDebugger.a in `build/`
2. libglfw3.a in `build/glfw/src/`

Copy them in your lib folder.
All include files you would need are stored in `graph-debugger/include`
To use the libraries, you can add following arguments to you compiler arguments
#### For Windows
```shell
-lGraphDebugger -lglfw3 -lgdi32 -lopengl32
```
#### For Linux
```shell
-lGraphDebugger -lglfw3
```

### For MSVC
Copy and paste the following lines in the shell.
```shell
git clone --recurse-submodules https://github.com/EnDeRGeaT/graph-debugger
cd graph-debugger
mkdir build
cd build
cmake ..
```
Then in build folder you can build the .sln file

After build has been completed, you will have two libraries you need. \
1. GraphDebugger.lib in `build/`
2. glfw3.lib in `build/glfw/src/`

Copy them in your lib folder.
All include files you would need are stored in `graph-debugger/include`
Dependencies that you need for the library to work are:
GraphDebugger.lib glfw3.lib gdi32.lib opengl32.lib

## Usage
To use the library your code can be as simple as that
```cpp
#include "GraphDebugger.h"
#include <vector>

int main(){
    auto edges = std::vector<std::pair<uint32_t, uint32_t>>{
        {0, 1},
        {1, 2},
        {1, 3}
    };
    auto g = Graph(4, edges);
    g.visualize();
}
```
To learn about the things you can do in the window itself, press H while focused on GraphDebugger window.