Small Graph drawing library which makes drawing graphs trivial!

![](https://cdn.discordapp.com/attachments/751095110246072322/1234564177251799060/2024-04-29_20-25-51.gif?ex=663130fc&is=662fdf7c&hm=b496f88401604f7061258f948035c974d1a998fe69a8867373417bf494be10ac&)

## Prerequisites
- CMake 3.0+
- Git 1.8.2+

## Build
### For GCC/Clang
If you use Linux, you need to install dependencies for GLFW.  
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
After build has been completed, you will have two libraries you need.
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

After build has been completed, you will have two libraries you need.   
1. GraphDebugger.lib in `build/`
2. glfw3.lib in `build/glfw/src/`

Copy them in your lib folder.  
All include files you would need are stored in `graph-debugger/include`  
Dependencies that you need for the library to work are:  
- GraphDebugger.lib glfw3.lib gdi32.lib opengl32.lib

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
    auto g = debug::Graph(edges);
    g.visualize();
}
```
To learn about the things you can do in the window itself, press H while focused on GraphDebugger window.
## Issues
Since the library tries to be as non-intrusive as possible, there are some sacrifices to be made.  

For example, this library is not supported on MacOS and will never be supported in an intended way (or in any way for that matter, OpenGL 4.3 is not supported on Apple products).

Also there is a bug on some of the Linux distros, which results in a segfault upon closing the window.

One of the workarounds is to visualize an empty Graph at the start of your main function in order for it to crash at the end (not a great way but oh well):
```cpp
int main(){
    auto blank_edges = std::vector<std::pair<uint32_t, uint32_t>>{};
    auto blank = debug::Graph(blank_edges);
    blank.visualize();
    /* your program as usual */
}
```

Another workaround which should (hopefully) work on any platform is to create and run GraphDebugger window in the main thread and run all of your code in another one.
```cpp

int main(){
    debug::Graph::initializeWindow(); // initializes the window

    std::thread th([&]() {
        /* your program as usual*/
    });

    debug::Graph::runWindowLoop(); // starts the main loop of the window
    th.join();
}
```
