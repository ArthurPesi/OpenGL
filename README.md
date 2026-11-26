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

These checks do not open a window and cover animation loading, playback,
collisions, and track positions/headings.

## Track-following animation

Run `./build/trackgen` from the project root, click at least four control points,
press Space to close the loop (optional), then press Esc to export both
`objects/track.obj` and `objects/track.anim`. The car in `config.cfg` uses
`7 objects/track.anim 1`.
An initial animation is included for the existing track; `car.anim` is retained
as the four-frame rotation example.

Each B-spline centerline sample produces one animation row. Positions use the
same centroid and flipped-Z pixel-to-world mapping as the road mesh, then apply
the track's scene scale of `6` and height of `-0.9`. The car retains its `0.12`
scale and turns around Y to align its local +Z front with the curve tangent.
Closed tracks use wrapping neighbors; open endpoints use one-sided tangents.
Repeated points use the next distinct sample, or the previous sample at the end.

The placement constants in `TrackPath.hpp` must match the track transform in
`config.cfg` if you change its scale or height. Playback still advances one
sample every 0.5 seconds (2 Hz), with no interpolation; open tracks jump back to
the beginning when playback loops. Run `./build/hexagon` to view the car following
the generated track. To view the original rotation example instead, change the
car record back to `7 car.anim 1`.
