// Single translation unit that compiles the stb_image implementation.
// Keeping STB_IMAGE_IMPLEMENTATION here (and nowhere else) avoids duplicate
// symbols and keeps the ~8k-line implementation out of main.cpp's rebuilds.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
