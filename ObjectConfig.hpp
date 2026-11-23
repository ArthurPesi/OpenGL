#pragma once

#include <istream>
#include <memory>
#include <string>
#include <vector>

#include "Obj3D.hpp"

// On failure, leaves transforms unchanged. Blank lines and comments are ignored.
bool readAnimation(std::istream &in, std::vector<glm::mat4> &transforms);

// Reads the fields after the mesh index; animation paths are relative to baseDir.
// Returns nullptr for invalid records or missing/invalid animation files.
std::unique_ptr<Obj3D> readObjectConfig(std::istream &in, const std::string &baseDir);
