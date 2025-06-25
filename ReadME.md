# aZero Engine

### Description
*aZero Engine* is a game engine API with the focus on simplicity and flexibility.

It's a work-in-progress remake of my old game engine found here: https://github.com/NoahSch1999/aZeroEngine-Project

It has several main interfaces which handle certain functionality:
 - Engine
	 - Main interface of the engine
	 - Provides access to the other interfaces
 - Renderer
	 - Handles rendering related functionality
 - Window
	 - Encapsulates a window and relevant functionality
 - AssetManager
	 - Enables asset loading and handling
 - Scene
	 - Stores game entities which are considered to be in the same scene

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
 - Per-batch instanced drawing which batches all draws based on unique materials and meshes
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
