What is it?
* An implementation of the novel non-linear sphere tracing rendering algorithm proposed by Seyb et al. (https://dl.acm.org/doi/10.1145/3355089.3356502)
* A tool for authoring, playing, loading, and storing animations using LBS (linear blend skinning) with SDF (signed distance field) geometry

How to use:
* Build the project using CMake (This has only been tested for Windows)
* (Optional) Edit the SDF geometry by describing it in shader code (bin/assets/shaders/sdf.glsl)
* Create a skeleton and adjust joint weights
* Create keyframes to interpolate between where you pose the skeleton
* Complete the animation and save it (so that it can be loaded at a later time)
