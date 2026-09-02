#include "Face.hpp"

void Face::push(int v, int t, int n) {
    verts.push_back(v);
    texts.push_back(t);
    norms.push_back(n);
}