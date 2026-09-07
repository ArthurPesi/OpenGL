#pragma once

#include <vector>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Obj3D.hpp"

class Projectile : public Obj3D {
public:
    static const float PROJECTILE_SPEED;

    Projectile(Mesh *mesh, glm::vec3 position, glm::vec3 direction);
    bool step(float deltaTime, std::vector<Obj3D> &objects);

private:
    glm::vec3 direction;
    glm::vec3 position;
    float remainingTime;
};