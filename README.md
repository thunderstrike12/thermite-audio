# Thermite

Thermite is our ray-traced voxel game engine that uses the **Graphite** render graph for a Year 3 team project at **Breda University of Applied Sciences**.

## Tech Stack (WIP)
- Modern C++20 architecture
- Graphite Render Graph
- CMake (project configuration)

## Getting Started (WIP)
Open up the root folder inside your IDE, preferably Visual Studio or VSCode because they have cmake support built-in. Upon opening, CMake will automatically run and configure.

### Adding a new project
To add a new project that automatically links against the engine static .lib, add a uniquely named folder inside ```projects/```, containing a ```main.cpp```. Reopen your IDE or reconfigure cmake inside your IDE to make it appear as a startup target. Select your project (the dropdown with the green play button, it will have the same name as the folder) and start coding with the thermite engine by including its headers.

### Selecting a configuration
In VSCode and Visual Studio, you can select any of the following presets:

- Debug-Editor
- Debug-Game
- Release-Editor
- Release-Game
- RelWithDebInfo-Editor
- RelWithDebInfo-Game

They define:
- Supplied compiler flags.
- Whether to compile with debug info.
- Whether the editor is included.
- The values of the macros ```THERMITE_DEBUG``` and ```THERMITE_EDITOR```.
 
