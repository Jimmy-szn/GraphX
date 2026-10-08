# GraphX

An iterative math visualization C++/OpenGL application that provides an interactive way to visualize mathematical functions and recursive fractal sets in real time. It allows students and lecturers to explore complex concepts visually, making mathematics easier to understand and more engaging.

## Group Members and responsibilities
1. Jimmy Kariuki - Rasterization, Intergration and Testing
2. Paul Obonyo - User Interface Design
3. Gerald Aduda- System Architecture and Mode management
4. Daniel Tashobya - 3D Surface engineering for equations
5. Tracy Mugure - 2D Fractal viewport engineering and GLSL shading
6. Naomi Teko- Camera Navigation and Interaction

## Features

- 3D and 2D explicit function plotting
- 2D fractal rendering with shaders

## Technologies and Tools Required

- C++17
- OpenGL 3.3 
- GLFW
- GLAD
- GLM
- CMake
- Ninja
- Dear ImGui
## Graphical Techniques and Algorithims
Planned use of Line, Circle and Polygon algorrithms:
1.  DDA/ Bresenham Line algoritm have been used to determine which pixels form a line in the 2D graph plotting.
2. Scan Line Polygon fill will be used to fill interiors of drawn polyons.
3. Midpoint Circle Algorithm has been used to plot circle pixels in Circle Plotting.
4. 3D Function plotting: Sample points will be connected to form  triangles and  displayed over the 3D surface as a filled mesh.
5. 2D Fractal plotting: Iteration of preset number equations for each point in the viewport and coloring points according to their escape iteration count.
## Build

cmake -s, B build -G Ninja
cmake --build build

.\build\GraphX.exe

## Implemented

- CPU pixel buffer and DDA/Bresenham line algorithms
- Midpoint circle drawing and scanline polygon filling
- 2D plotting controls for line, parabola, sine, cosine, and circle
- Coordinate grid, axes, and numeric axis labels
- Optional fill beneath a 2D curve
- Snapshot Saving

## Inprogress and Limitation
- 3D Explicit function plotting
- 2D Fractals Plotting
- Camera navigation like zooming, e.t.c
- 2D Graph plotting uses preset functions, arbitrary equation input not yet supported





