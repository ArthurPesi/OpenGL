#include "Obj3D.hpp"

#include <cmath>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

const glm::mat4 &Obj3D::getTransform() const {
    return transform;
}

void Obj3D::updateAnimation(double /*elapsedSeconds*/) {
}

void Obj3D::init() {
    if (mesh == nullptr) return;
    const glm::mat4 &objectTransform = getTransform();
    glm::vec3 corners[8];
    for (int i = 0; i < 8; i++) {
        glm::vec3 corner(
            (i & 1) ? mesh->max.x : mesh->min.x,
            (i & 2) ? mesh->max.y : mesh->min.y,
            (i & 4) ? mesh->max.z : mesh->min.z);
        glm::vec4 transformed = objectTransform * glm::vec4(corner, 1.0f);
        corners[i] = glm::vec3(transformed);
    }
    worldMin = corners[0];
    worldMax = corners[0];
    for (int i = 1; i < 8; i++) {
        worldMin = glm::min(worldMin, corners[i]);
        worldMax = glm::max(worldMax, corners[i]);
    }
}

bool Obj3D::checkSphereCollision(const glm::vec3 &p, float r, glm::vec3 &normal, glm::vec3 &closest) const {
    if (mesh == nullptr) {
        return false;
    }
    closest = glm::clamp(p, worldMin, worldMax);
    glm::vec3 diff = p - closest;
    float distSq = glm::dot(diff, diff);
    if (distSq > r * r) {//No collision
        return false;
    } else { //Collision
        normal = diff / std::sqrt(distSq);
        return true;
    }
}
