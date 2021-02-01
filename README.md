# HPWHsim

An open source simulation model for Heat Pump Water Heaters (HPWH).

HPWHsim was developed with whole house simulation in mind; it is intended to be run independently of the overarching simulation's time steps, other parameters, and does not aggregate its own outputs. It was also designed to run quickly, as the typical use case would see many simulations run, each a year-long or more.

### Development

HPWHsim is configured as a CMake project. Currently, CMake is only configured to generate Microsoft Visual Studio solutions compiled with Microsoft Visual C++ (other generators and compilers will not work). CMake also handles version control via Git.

### Dependencies

- Microsoft Visual Studio 2017 with Visual C++ (which can be installed after from the Microsoft Visual Studio Installer)
- CMake 3.5 or later
- Git

### Building HPWHsim from source

1. Clone the git repository, or download and extract the source code.
2. Make a directory called `build` inside the top level of your source.
3. Open a console in the `build` directory.
4. Type `cmake ..`.
5. Type `cmake --build . --config Release`.
6. Type `ctest -C Release` to run the test suite and ensure that your build is working properly.
