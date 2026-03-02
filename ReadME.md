# aZero Engine

### Description
*aZero Engine* is a game engine API with the focus on simplicity and flexibility.

It's a work-in-progress remake of my old game engine found here: https://github.com/NoahSch1999/aZeroEngine-Project

The idea is that the API provides easy-to-use and flexible engine-functionalities. Then the user can do whatever they want with it.

The API doesn't provide a build-in window-creation since the idea is to make it as flexible as possible. Instead of having a window class, the API provides functionality to copy a rendered frame to your swapchain.

### Classes
*Core classes*
 - Engine
	 - Main interface of the engine.
	 - Provides access to the other interfaces.

*Core api interfaces*
 - Scene
	 - Interface to store and modify entities in your scene.
 - Renderer
	 - Interface for all render-related functionality such as rendering scenes, render-settings, updating the renderstate of entities, and more...
 - RenderTarget
     - Interface that represents a color render target.
 - DepthStencilTarget
     - Interface that represents a depth and stencil render target.
 - Swapchain
     - Interface that connects the rendered frames to the application window.

### Usage Guide
SETUP GUIDE FOR VISUAL STUDIO

Requires at least Windows Kits 10.0.22621.0. Try installing the most up-to-date version of VS2022.

1. Clone repo

2. Right-click aZeroEditor folder and choose "Open with Visual Studio"

3. If the top-level directory doesn't have an "out" folder, CTRL+S the top-level CMakeLists.txt in Visual Studio

4. Build all with F7

5. Run with F5 

### Technical Features

 - Asset loading (mesh, texture, etc)
 - Dynamic amount of point, spot, and directional lights with frustrum culling for the first two
 - Per-batch instanced drawing that batches all draws based on unique materials and meshes
 - Normal mapping
 - In-engine HLSL shader compilation and reflection enabling name-based pipeline binding

### Planned Features
Already in my old project but has to be implemented here:

 - Deferred rendering (will probably be skipped for a more modern rendering technique)
 - Per-entity outlines (post process)
 - Color-based entity picking (optional)
 - Scene save/load
 - Material transparency maps
 - Entity parenting
 - Directional shadow mapping
 - Scene and material editor (incl. material drag/drop)
 
Other:
 - PBR shading
 - Downsampled-style glow (such as Unity's)
 - Compute-based skeletal animation and relevant components
 - Rigidbody component
 - Audio source component
 - Octree culling
 - Render graph for a more automatic render pass configuration
