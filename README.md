# Dependencies
CMake, OpenGL, GLFW3, GLEW

# Build and Run

## macOS

```sh
brew install glfw glew glm
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build
./build/hexagon
```

## Linux

```sh
cmake -S . -B build
cmake --build build
./build/hexagon
```

## Windows (MSVC)

```sh
cmake -S . -B build
cmake --build build --config Release
build\Release\hexagon.exe
```

## Windows (MSYS2)

```sh
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
./build/hexagon.exe
```

# Transform animations

In the transform section of `config.cfg`, an alphabetic first character after the
mesh index selects an animation filename instead of a static transform:

```text
7 car.anim 1
```

The collision flag is still required (`0` = destructible, `1` = reflect).
Animation paths are relative to the config file's directory. Each animation row
contains `Tx Ty Tz Sx Sy Sz Rx Ry Rz`, with rotations in degrees, for example:

```text
0.0 -0.9 0.0 0.12 0.12 0.12 0 0 0
0.0 -0.9 0.0 0.12 0.12 0.12 0 90 0
```

Frames play in file order, starting at zero, and loop without interpolation.
`GLOBAL_ANIMATION_FREQUENCY` in `AnimatedObj3D.hpp` defaults to 2 Hz (one frame
every 0.5 seconds). Blank lines and `#` comments are ignored; missing, empty, or
malformed animations are reported and the affected object is skipped.

## Nonvisual checks

```sh
cmake --build build
ctest --test-dir build --output-on-failure
```

These checks do not open a window. For a manual visual check, run
`./build/hexagon` from the project root and look at the sample car: it should
cycle through 0°, 90°, 180°, and 270°, changing pose every 0.5 seconds and
repeating every 2 seconds.
