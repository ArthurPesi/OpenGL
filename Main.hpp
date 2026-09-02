#pragma once
#include <vector>

#include "Mesh.hpp"

class Main {
public:
    std::vector<Mesh> meshes;
    void drawScene();
    void main();
};