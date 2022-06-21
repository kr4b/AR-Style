Coherent Stylization for Stereoscopic Augmented Reality
======================================================

Code repository for research on `Coherent Stylization for Stereoscopic Augmented Reality`, as part of the [TU Delft Research Project](https://cse3000-research-project.github.io/2022/Q4).
More specifically, this research is a sub-project of the project `Media of the Future`.
The respective research paper can be found [here](http://resolver.tudelft.nl/uuid:c5b8ae9a-7e0b-412f-ab12-40d9f9613e5b).

This project makes use of [OpenCV](https://opencv.org/) and [ARToolkitX](https://github.com/artoolkitx/artoolkitx).

Most of the contributed code for this project is located in `Examples/Stereo Stylization Coherence`.
The most prominent code related to the research can be found in `draw.cpp` and `voronoi.hpp` in the aforementioned folder.

- `voronoi.hpp` - Contains code related to generating the voronoi pattern, relocating anchor points and redistributing the density.
- `draw.cpp` - Contains code related to coordinating the rendering and generating the quantitative results.

## Usage

The code was developed and tested on a Linux system.
It might work on Windows with a bit of effort, and macOS with a bit more effort.
With a lot of effort it could probably even work on Android and/or iOS systems, but the OpenGL code is not fully converted to OpenGL ES.

The video frames required to run the application are not provided with this project, but should be placed in the `video` directory with the following naming convention:

- Frame 0 left: 0000L.jpg
- Frame 0 right: 0000R.jpg
- ...
- Frame 100 left: 0100L.jpg
- Frame 100 right: 0100R.jpg

