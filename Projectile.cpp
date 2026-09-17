#include "Projectile.hpp"

const float Projectile::PROJECTILE_SPEED = 10.0f;

Projectile::Projectile(Mesh *mesh, glm::vec3 position, glm::vec3 direction)
    : direction(direction), position(position), remainingTime(1.2f) {
    this->mesh = mesh;
    this->direction = glm::normalize(direction);
    this->collision = false;
    this->transform = glm::translate(glm::mat4(1.0f), this->position);
}

bool Projectile::step(float deltaTime, std::vector<Obj3D> &objects) {
    position += direction * PROJECTILE_SPEED * deltaTime;
    remainingTime -= deltaTime;
    transform = glm::translate(glm::mat4(1.0f), position);

    float radius = (mesh != nullptr) ? (mesh->max.x - mesh->min.x) * 0.5f : 0.5f;
    for (const auto &obj : objects) {
        glm::vec3 normal;
        glm::vec3 closest;
        if (obj.checkSphereCollision(position, radius, normal, closest)) {
            direction = glm::normalize(direction - 2.0f * normal * glm::dot(normal, direction));
            position = closest + normal * (radius + 1e-3f);
            transform = glm::translate(glm::mat4(1.0f), position);
            break;
        }
    }
    return remainingTime <= 0.0f;
}
