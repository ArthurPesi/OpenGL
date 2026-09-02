#pragma once
#include <string>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Group.hpp"

class Mesh {
public:
    std::string mtllib;
    std::vector<Group*> groups;
    std::vector<glm::vec3*> vertex;
    std::vector<glm::vec2*> texts;
    std::vector<glm::vec3*> normals;
};
