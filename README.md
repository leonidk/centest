# census test
Testing of Census-like stereo algorithms. Mostly a space for experimenting with small concepts and ideas. 
As such, the code has basically no comments and poor interfaces for most of it's functionality.

## Algorithms Implemented
* SSD Block Matching (with an optional Sobel prefilter), for Left + Right frame perspective, with quadratic subpixel estimation
* Census Matching (24 bit, 7x7 descriptor) + Block Matching, for Left + Right frame perspective, with linear subpixel estimation
* Census (left frame only) with thresholds from the [Intel RealSense R200](https://github.com/IntelRealSense/librealsense/blob/master/include/librealsense/rsutil.h). Implemented as documented in [Intel's released documentation](https://github.com/PercATI/RealSense_ROS/blob/master/r200_install/Include/DSAPI/DSAPITypes.h)
* Semiglobal Matching (5 paths), with SAD + Census cost metrics, subpixel matching, R200 thresholds, and bilateral filter window weights. Only on the left-frame perspective.

## Dependencies
* GLFW3 (not used in any code path as of April 2016)

## Building
### Windows
* Pull down a version of this repository
* A Visual Studio 2013 Solution is included in msvc/

### Linux
* Pull down a version of this repository.
* I'm using the amazingly convenient [Generic Makefile](https://github.com/mbcrawfo/GenericMakefile).
* It has all the dependencies specified through pkg-config, see line 11 of the Makefile (gl & glfw3) 
* Type _make_ in the top-level-centest. _make debug_ also works. This'll build everything and make a symlink to ./centest.
* Run via ./centest <left image> <right image>

## License
Mozilla Public License 2.0
