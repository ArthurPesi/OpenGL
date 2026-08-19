# Dependencies
CMake, OpenGL, GLFW3, GLEW

# Build and Run

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
