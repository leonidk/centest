# census test
Testing of Census-like stereo algorithms. Mostly a space for experimenting with small concepts and ideas. 
As such, the code has basically no comments and poor interfaces for most of it's functionality.

## Dependencies
* TODO

## Building
### Windows
* Pull down a version of this repository
* A Visual Studio 2013 Solution is included in msvc/


### Linux
* Pull down a version of this repository.
* I'm using the amazingly convenient [Generic Makefile](https://github.com/mbcrawfo/GenericMakefile).
* It has all the dependencies specified through pkg-config, see line 11 of the Makefile. 
* Type _make_ in the top-level-centest. _make debug_ also works. This'll build everything and make a symlink to ./ps1080.
* Run via ./centest

## License
Mozilla Public License 2.0