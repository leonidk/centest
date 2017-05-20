# census test
Testing of Census-like stereo algorithms. Mostly a space for experimenting with small concepts and ideas. 
As such, the code has basically no comments and poor interfaces for most of it's functionality.

## Algorithms Implemented
* SSD Block Matching (with an optional Sobel prefilter), for Left + Right frame perspective, with quadratic subpixel estimation
* Census Matching (24 bit, 7x7 descriptor) + Block Matching, for Left + Right frame perspective, with linear subpixel estimation
* Census (left frame only) with thresholds from the [Intel RealSense R200](https://github.com/IntelRealSense/librealsense/blob/master/include/librealsense/rsutil.h). Implemented as documented in [Intel's released documentation](https://github.com/PercATI/RealSense_ROS/blob/master/r200_install/Include/DSAPI/DSAPITypes.h). Optionally includes a domain-transform on the cost volume. [As published](https://arxiv.org/abs/1705.05548) these thresholds can be effective in removing spurious matches. 
* Semiglobal Matching (5 paths), with SAD + Census cost metrics, subpixel matching, R200 thresholds, discontinuity scaling for SGM, naive hole filling and bilateral filter window weights. Only on the left-frame perspective.

## Dependencies
* C++14 compliant compiler
* GLFW3 for visualization
* librealsense for librealsense executable

## Building
### Windows
* Pull down a version of this repository
* A Visual Studio 2015 Solution is included in msvc/
* CensusMatching is the primary project of interest
* GLFW3 is installed via a NuGet Package, so Visual Studio should pull it down automatically 

### Linux
* Pull down a version of this repository
* Type `make` in the top-level-centest. This'll build everything and generate binaries in the Makefile directory.
* Makefile can be modified to build a debug version of the executable

## Running
* Run via `./centest <json>`
* An example would be `./centest github_census/github_census.json`
* The example code expects to have a converted version of [Middlebury's v3 dataset](http://vision.middlebury.edu/stereo/submit3/)
* The conversion routine is on github_census/generate_middlebury.py. This is a python script that requires Pillow and Imagemagick.
* This program also supports passing valid json on the command line, instead of a file. 
* Output results are in the current working directory, to an 32-bit float disparity image PFM, and a confidence map in 32-bit floating point. 
* To visualize the pfm files, a vis_pfm script is provided `./vis_pfm depth.pfm [conf.pfm]` with an optional conf.pfm argument, which generates an out.png file the execution directory with a histogram colored depthmap.

## License
Mozilla Public License 2.0. More information is available on the [Wikpedia article on MPL](https://en.wikipedia.org/wiki/Mozilla_Public_License) or [MPL's official FAQ](https://www.mozilla.org/en-US/MPL/2.0/FAQ/). 
