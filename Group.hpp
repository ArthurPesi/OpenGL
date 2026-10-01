#pragma once
#include <string>
#include <vector>

#include <GL/glew.h>

#include "Face.hpp"

class Group {
public:
    std::vector<Face> faces;
    std::string material;   // usemtl name; resolved to a Material* at draw time
    GLuint VAO = 0;
    GLuint VBOs[3] = {0, 0, 0};  // position / texcoord / normal buffers (for cleanup)
    int numberOfVertices = 0;
};