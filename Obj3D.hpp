#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Mesh.hpp"

class Obj3D {
public:
    glm::mat4 transform = glm::mat4(1.0f);
    Mesh *mesh = nullptr;
    bool reflect = false;

    bool checkSphereCollision(const glm::vec3 &p, float r, glm::vec3 &normal, glm::vec3 &closest) const;
};
