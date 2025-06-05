# Terrain Rendering with Airplane Simulation

## Overview
This project is a 3D terrain rendering application with an interactive airplane simulation. The terrain generation was inspired by the OGLDEV YouTube channel tutorials, which provided valuable insights into terrain rendering techniques.

## Technologies Used
- OpenGL 3.3+
- GLFW for window management
- GLEW for OpenGL extension loading
- ImGui for user interface
- C++ for core functionality

## Features
- Procedurally generated terrain using midpoint displacement algorithm
- Interactive airplane simulation with realistic controls
- Dynamic camera system with multiple viewing modes
- Bird flocking simulation
- Real-time lighting and shading
- Customizable terrain parameters
- Interactive GUI for real-time adjustments

## Controls

### Camera Modes
- Press `F` to toggle free camera mode
- Press `T` to enter airplane-following camera mode

### Airplane Controls (when in airplane-following mode)
- `W` - Move forward
- `S` - Move backward
- `A` - Move left
- `D` - Move right
- `+` - Move up
- `-` - Move down
- `V/B` - Rotate left/right
- `N/M` - Pitch up/down
- `K/L` - Roll left/right

### General Controls
- `ESC` or `Q` - Exit application
- `R` - Reset camera position
- `C` - Print camera position
- `E` - Toggle wireframe mode
- `P` - Pause simulation
- `SPACE` - Toggle GUI
- `0` - Toggle point display
- `1/2/3` - Change cube follow mode

## Customization Options

### Terrain
- Adjust terrain size in `terrain_demo1.cpp`:
  - Modify `m_terrainSize` (default: 513)
  - Change `m_roughness` for different terrain patterns
  - Adjust `m_minHeight` and `m_maxHeight` for elevation range

### Birds
- Modify bird count in `InitBirds()` function
- Adjust bird speed and behavior in the `Bird` class
- Change bird appearance by modifying the `RenderBirdPart` function

### Lighting
- Adjust light direction in `InitTerrain()` function
- Modify light intensity and color in `terrain.fs` shader
- Customize ambient and diffuse lighting parameters

### Airplane
- Change airplane size by modifying `cubeSize` in `InitPlayerCube()`
- Adjust airplane speed by changing `m_cubeSpeed`
- Modify camera distance and height offset in `SetCameraBehindCubeFixed()`

## Development Notes
- The project uses modern OpenGL practices with shader-based rendering
- Terrain generation uses the midpoint displacement algorithm for natural-looking landscapes
- The airplane simulation includes realistic physics and collision detection
- The bird flocking system implements basic flocking behavior

## Requirements
- C++ compiler with C++11 support
- OpenGL 3.3 or higher
- GLFW 3.x
- GLEW
- ImGui

## Building
1. Ensure all dependencies are installed
2. Compile using your preferred build system
3. Run the executable

## Credits
- Terrain generation techniques inspired by OGLDEV YouTube channel
- OpenGL and GLFW for graphics and window management
- ImGui for the user interface system 