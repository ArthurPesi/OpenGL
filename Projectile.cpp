#include "Projectile.hpp"

#include <glm/geometric.hpp>

const float Projectile::PROJECTILE_SPEED = 22.0f;
const float Projectile::PROJECTILE_LIFETIME = 1.2f;
const float Projectile::RADIUS = 0.25f;

Projectile::Projectile(Mesh *mesh, glm::vec3 position, glm::vec3 direction)
    : position(position), remainingTime(PROJECTILE_LIFETIME) {
    this->mesh = mesh;
    this->direction = glm::normalize(direction);
    this->reflect = false;
    this->transform = glm::translate(glm::mat4(1.0f), this->position);
}

bool Projectile::step(float deltaTime, std::vector<Obj3D> &objects) {
    position += direction * PROJECTILE_SPEED * deltaTime;
    remainingTime -= deltaTime;
    transform = glm::translate(glm::mat4(1.0f), position);

    for (auto it = objects.begin(); it != objects.end(); ++it) {
        glm::vec3 normal;
        glm::vec3 closest;
        if (it->checkSphereCollision(position, RADIUS, normal, closest)) {
            if (it->reflect) {
                // Objeto sólido: o projétil quica (reflete a direção).
                direction = glm::normalize(glm::reflect(direction, normal));
                position = closest + normal * (RADIUS + 1e-3f);
                transform = glm::translate(glm::mat4(1.0f), position);
                break;
            } else {
                // Objeto destrutível: remove o objeto e o projétil.
                objects.erase(it);
                return true;
            }
        }
    }
    return remainingTime <= 0.0f;
}
