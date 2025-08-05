# Visualizing 3D Ray-Traced Scenes with an In-House Interactive Ray Tracer

<img align="center" src="https://dlsu-game-lab.github.io/AnitoTracer/assets/images/banner.png">

<h1>Trailer</h1>
<video style="width:100%; height:auto;" autoplay muted controls>
    <source src="https://dlsu-game-lab.github.io/AnitoTracer/assets/videos/anito-trailer.mp4" type="video/mp4">
</video>

AnitoTracer is an in-house real-time and interactive ray tracing 3D scene editor that can potentially be used by beginner 3D level designers and game designers in exploring how their scenes would appear in a ray traced environment. Its code is open source to allow for other artists and programmers to freely extend the capabilities of the ray tracer to suit their needs.

The project is being developed by undergraduate students in De La Salle University as part of their capstone. The members of Waffle Shader Studios are as follows:
    <ul>
        <li>Kate Nicole Young</li>
        <li>Shane Laurenze Cablayan</li>
        <li>Marcus Rene Levin Leocario</li>
        <li>Zachary Gadjiel Breinard Que</li>
        <li>Andre Vito Valdecantos</li>
    </ul>

Under the advisory of Neil Patrick Del Gallego, Ph.D.

<h3>Abstract</h3>
      
Ray tracing is one of the standard 3D rendering practices in video games and computer graphics due to its capability to produce photorealistic renders. 
The existing 3D scene editors in the industry, like Unity, Unreal, and Blender, do not support real-time raytracing interactivity, and users would have 
to alter the editor's pipeline settings in order to render a raytraced scene. The study shows that the developed in-house real-time ray tracing 3D engine 
received a Standard Usability (SUS) Score of 68.33, indicating that the engine is usable in its current form and provides a solid foundation for further 
development. There is potential for improvement in areas such as optimization, stability, and pixel-perfect object picking. Additionally, as the in-house 
ray tracer, AnitoTracer, is open source, artists and programmers can freely extend the capabilities of the base ray tracer to further improve on it and suit 
their needs. 

<h1>Key Features</h1>
<h2>Ray Tracing</h2>
<img src="https://dlsu-game-lab.github.io/AnitoTracer/assets/images/ray_tracing_feature.png" alt="Ray Tracing Feature" style="width:100%; height:auto;">
    AnitoTracer is a ray tracing engine that is designed to replicate how light behaves in the real world by simulating rays that bounce or pass through materials 
    of 3D models in a scene, producing highly realistic images with detailed reflections, refractions, and shadows.

<h2>Ray Visualization</h2>
<img src="https://dlsu-game-lab.github.io/AnitoTracer/assets/images/ray_visualization_feature.jpg" alt="Ray Visualization Feature" style="width:100%; height:auto;">

    AnitoTracer includes a built-in ray visualization feature that depicts how rays travel through a scene, highlighting their paths as they interact with various materials,
    bounce off different surfaces, and how it ultimately contributes to the final color rendered on the screen.

<h2>Scene Editor</h2>
<img src="https://dlsu-game-lab.github.io/AnitoTracer/assets/images/scene_editor_feature.png" alt="Scene Editor Feature" style="width:100%; height:auto;">

    AnitoTracer has an integrated scene editor that enables users to load in scenes, modify objects, adjust materials, and create custom lighting setups allowing for easy
    experimentation of raytraced rendering.

<h1>Build Instructions</h1>
<h2>With CMake</h2>
1. Run "vcpkg_windows.bat". This installs vcpkg and other dependencies.
2. Build project using CMake and included "CMakeLists.txt" file.

<h1>Download</h1>

You can download the software from this project's <a href="https://drive.google.com/file/d/1koymLbZgpnqmexP2hAP4qkE3cz2nphTD/view?usp=sharing" target="_blank">Google Drive Link</a>.
