#pragma once
#include <vector>

class Face {
public:
    std::vector<int> verts;
    std::vector<int> norms;
    std::vector<int> texts;

    void push(int, int, int);
};