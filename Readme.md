RayReflect
==========
RayReflect is a small renderer that uses rays to display reflections on objects.

Compilation
----------
RayReflect can be compiled on Windows using MinGW.  It may be able to be compiled with Clang/Clang++, but compilation has not been tested with Clang.  
It will not compile with MSVC, as MSVC is not fully C99 complient.

To compile make a directory subdirectory bin.  Then in bun run  
```
gcc -c ../*.c -DNDEBUG -Wfatal-errors -O3 -I.. -I../include
g++ -c ../*.cpp -DNDEBUG -Wfatal-errors -O3 -I.. -I../include -std=c++11
g++ *.o -o ../bin/release/ray.exe -L../lib -lglfw3 -lgdi32 -lvulkan-1 -mwindows
```

Dependencies
-----------
All files needed for compilation should be included.
External libraries used are:  
GLFW3  
OpenGL Mathematics(GLM)  
Vulkan

If vulkan is installed on your system, the program should run.

Running
______
To run, copy the res folder to the bin folder and copy the contents of shaders/bin to a subdirectory of bin called shaders.  
The folder structure should look like this:
res  
____*.png  
shaders  
____*.vert  
____*.frag  
____bin  
________*.spv  
bin  
____ray.exe  
____res  
________*.png  
____shaders  
________*.spv
