#pragma once

#include <cstddef>
#include <vector>

#include "Obj3D.hpp"

inline constexpr double GLOBAL_ANIMATION_FREQUENCY = 14.0; // frames per second (Hz)

class AnimatedObj3D : public Obj3D {
public:
    std::vector<glm::mat4> transforms;

    const glm::mat4 &getTransform() const override;
    void updateAnimation(double elapsedSeconds) override;

private:
    std::size_t currentFrame = 0;
};
