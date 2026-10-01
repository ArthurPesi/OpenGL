#include "Obj3D.hpp"

#include <cmath>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

bool Obj3D::checkSphereCollision(const glm::vec3 &p, float r, glm::vec3 &normal, glm::vec3 &closest) const {
    if (mesh == nullptr) {
        return false;
    }
    glm::vec3 corners[8];
    for (int i = 0; i < 8; i++) {
        glm::vec3 corner(
            (i & 1) ? mesh->max.x : mesh->min.x,
            (i & 2) ? mesh->max.y : mesh->min.y,
            (i & 4) ? mesh->max.z : mesh->min.z);
        glm::vec4 transformed = transform * glm::vec4(corner, 1.0f);
        corners[i] = glm::vec3(transformed);
    }
    glm::vec3 worldMin = corners[0];
    glm::vec3 worldMax = corners[0];
    for (int i = 1; i < 8; i++) {
        worldMin = glm::min(worldMin, corners[i]);
        worldMax = glm::max(worldMax, corners[i]);
    }
    closest = glm::clamp(p, worldMin, worldMax);
    glm::vec3 diff = p - closest;
    float distSq = glm::dot(diff, diff);
    if (distSq > r * r) {
        return false;
    }
    const float eps = 1e-6f;
    if (distSq > eps * eps) {
        normal = diff / std::sqrt(distSq);
        return true;
    }
    closest += glm::vec3(0.5f) * (worldMin + worldMax);
    glm::vec3 c = p - closest;
    glm::vec3 half = 0.5f * (worldMax - worldMin);
    float ax = std::abs(c.x) / (half.x > eps ? half.x : 1.0f);
    float ay = std::abs(c.y) / (half.y > eps ? half.y : 1.0f);
    float az = std::abs(c.z) / (half.z > eps ? half.z : 1.0f);
    if (ax >= ay && ax >= az) {
        closest = glm::vec3(c.x >= 0.0f ? worldMax.x : worldMin.x, p.y, p.z);
        normal = glm::vec3(c.x >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
    } else if (ay >= az) {
        closest = glm::vec3(p.x, c.y >= 0.0f ? worldMax.y : worldMin.y, p.z);
        normal = glm::vec3(0.0f, c.y >= 0.0f ? 1.0f : -1.0f, 0.0f);
    } else {
        closest = glm::vec3(p.x, p.y, c.z >= 0.0f ? worldMax.z : worldMin.z);
        normal = glm::vec3(0.0f, 0.0f, c.z >= 0.0f ? 1.0f : -1.0f);
    }
    return true;
}