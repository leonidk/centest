# ps1080test
Testing on top of an OpenNI camera. Mostly a space for experimenting with small concepts and ideas. 
As such, the code has basically no comments and poor interfaces for most of it's functionality.

## Dependencies
Currently the code has a few MIT/BSD licensed, inlined dependencies, such as [linalg.h](https://github.com/sgorsten/linalg) 
and [nanoflann](https://github.com/jlblancoc/nanoflann). 
Outside of that, the following open-source libraries are required from additional sources:

* *OpenNI2*: For camera access, it's expected that you have an installation of OpenNI2 for your platform of interest. 
[Binaries are here](http://structure.io/openni) while the entire project 
[is also on github](https://github.com/occipital/openni2).
It's only used in ONICamera.h source files. 
* *SDL2*: This is needed as an abstraction layer for window handling across platforms. 
Download the development libraries, or their source code, 
[from their website](https://www.libsdl.org/download-2.0.php).
It's only used in the Drawing_*_SDL.h source files, and main.cpp
* *GLEW*: This is needed to get access to modern OpenGL functions on Windows.
[Binaries are here](http://glew.sourceforge.net/), 
while the [source code is on github](https://github.com/nigels-com/glew).
* *Eigen3*: A popular and flexible header-only linear algebra library. 
It's fairly large, at ~3MB extracted, so I tend to store it in a common location. 
It's only used for Cholesky decomposition in ICP.h. And a commented-out SVD-based pairwise matcher in detect.h.
[Releases are here](http://eigen.tuxfamily.org/index.php?title=Main_Page), 
while the [repository is here](https://bitbucket.org/eigen/eigen/).

## Building
### Windows
* Pull down a version of this repository
* A Visual Studio 2013 Solution is included in msvc/. All the dependencies are setup for x86 and x64, 
they just require locations of the aforementioned libraries, which are configured through editing a single property sheet, 
msvc/props/Config.prop.
* By default, you can just download the dependencies from the website and extract them to msvc/deps/. 
For example, I have msvc/deps/SDL2, msvc/deps/glew and msvc/deps/eigen, 
which are just extracted versions of the zip files from the websites
* Load the Solution and Build. 

### Linux
* Pull down a version of this repository.
* I'm using the amazingly convenient [Generic Makefile](https://github.com/mbcrawfo/GenericMakefile).
* It has all the dependencies specified through pkg-config, see line 11 of the Makefile. 
Install them through your distribution's package manager.
 * On Ubuntu: _sudo apt-get install libopenni2-dev libsdl2-dev libglew-dev libeigen3-dev_
 * On Fedora: _su -c 'yum install SDL2-devel glew-devel eigen3-devel'_ 
Unfortunately, [Fedora lacks an OpenNI2 pkg](https://bugzilla.redhat.com/show_bug.cgi?id=1060920),
and the default OpenNI2 Linux installer doesn't include a pkg-config file. 
[Debian maintainers have one](https://github.com/atuleu/debian-openni2/blob/master/debian/libopenni2.pc), 
but its Include directory has to be changed manually if used with the OpenNI2 install script. 
* Type _make_ in the top-level-directory. _make debug_ also works. This'll build everything and make a symlink to ./ps1080.
* Run via ./ps1080

### Mac OS X
* I haven't tried it. I've heard that [this guide](http://joseph-long.com/writing/sdl-and-os-x/) 
may be helpful in installing SDL2.


## License
Mozilla Public License 2.0
