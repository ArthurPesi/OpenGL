#pragma once
#include <vector>

#include <GL/glew.h>

#include "Face.hpp"

class Group {
public:
    std::vector<Face*> faces;
    std::string material;   // usemtl name; resolved to a Material* at draw time
    GLuint VAO;
    int numberOfVertices;
};