# census test
Testing of Census-like stereo algorithms. Mostly a space for experimenting with small concepts and ideas. 
As such, the code has basically no comments and poor interfaces for most of it's functionality.

## Algorithms Implemented
* SSD Block Matching (with an optional Sobel prefilter), for Left + Right frame perspective, with quadratic subpixel estimation
* Census Matching (24 bit, 7x7 descriptor) + Block Matching, for Left + Right frame perspective, with linear subpixel estimation
* Census (left frame only) with thresholds from the [Intel RealSense R200](https://github.com/IntelRealSense/librealsense/blob/master/include/librealsense/rsutil.h). Implemented as documented in [Intel's released documentation](https://github.com/PercATI/RealSense_ROS/blob/master/r200_install/Include/DSAPI/DSAPITypes.h)
* Semiglobal Matching (5 paths), with SAD + Census cost metrics, subpixel matching, R200 thresholds, discontinuity scaling for SGM, naive hole filling and bilateral filter window weights. Only on the left-frame perspective.

## Dependencies
* C++11 compliant compiler
* GLFW3 (not used in any code path as of April 2016)

## Building
### Windows
* Pull down a version of this repository
* A Visual Studio 2013 Solution is included in msvc/
* CensusMatching is the primary project of interest
* GLFW3 is installed via a NuGet Package, so Visual Studio should pull it down automatically. 

### Linux
* Pull down a version of this repository.
* I'm using the amazingly convenient [Generic Makefile](https://github.com/mbcrawfo/GenericMakefile).
* It has all the dependencies specified through pkg-config, see line 11 of the Makefile (gl & glfw3) 
* Type `make` in the top-level-centest. `make debug` also works. This'll build everything and make a symlink to ./centest.

## Running
* Run via `./centest <left image> <right image>`
* An example would be `./centest cones/im2.png cones/im6.png`
* Output results are in the current working directory, to an 8-bit disparity image PNG `disp-out.png`. 
* There is a selectable subpixel multiplier for the disparity image, and a full 16-bit disparity image is computed at whatever scaling is desired. Change `src\Main.cpp` if you want to change algorithms, subpixel multipliers, or disparity search range. Individual algorithm configurations are currently done as C-style macros at the top of each algorithm's cpp file. 

## License
Mozilla Public License 2.0
