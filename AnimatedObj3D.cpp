#include "AnimatedObj3D.hpp"

#include <algorithm>

const glm::mat4 &AnimatedObj3D::getTransform() const {
    if (transforms.empty()) return Obj3D::getTransform();
    return transforms[currentFrame % transforms.size()];
}

void AnimatedObj3D::updateAnimation(double elapsedSeconds) {
    if (transforms.empty()) return;
    std::size_t nextFrame = static_cast<std::size_t>(
        std::max(0.0, elapsedSeconds) * GLOBAL_ANIMATION_FREQUENCY) % transforms.size();
    if (nextFrame != currentFrame) {
        currentFrame = nextFrame;
        init(); // Keep collision bounds aligned with the displayed pose.
    }
}
