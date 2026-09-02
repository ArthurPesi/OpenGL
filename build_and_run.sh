#!/bin/sh

cmake -S . -B build
cmake --build build

exec ./build/hexagon
