#pragma once
#include <vector>

#include <GL/glew.h>

#include "Face.hpp"

class Group {
public:
    std::vector<Face*> faces;
    GLuint VAO;
    int numberOfVertices;
};