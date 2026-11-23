#include "ObjectConfig.hpp"

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

#include <glm/gtc/matrix_transform.hpp>

#include "AnimatedObj3D.hpp"

namespace {

bool readTransform(std::istream &in, glm::mat4 &transform) {
    float tx, ty, tz, sx, sy, sz, rx, ry, rz;
    if (!(in >> tx >> ty >> tz >> sx >> sy >> sz >> rx >> ry >> rz)) return false;

    glm::mat4 rotation(1.0f);
    rotation = glm::rotate(rotation, glm::radians(rx), glm::vec3(1.0f, 0.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(ry), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(rz), glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::translate(glm::mat4(1.0f), glm::vec3(tx, ty, tz)) * rotation *
                glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, sz));
    return true;
}

bool endOfRecord(std::istream &in) {
    in >> std::ws;
    return in.eof() || in.peek() == '#';
}

} // namespace

bool readAnimation(std::istream &in, std::vector<glm::mat4> &transforms) {
    std::vector<glm::mat4> frames;
    std::string line;
    while (std::getline(in, line)) {
        std::stringstream sline(line);
        if (endOfRecord(sline)) continue;

        glm::mat4 transform;
        if (!readTransform(sline, transform) || !endOfRecord(sline)) return false;
        frames.push_back(transform);
    }
    if (in.bad() || frames.empty()) return false;
    transforms = std::move(frames);
    return true;
}

std::unique_ptr<Obj3D> readObjectConfig(std::istream &in, const std::string &baseDir) {
    in >> std::ws;
    int next = in.peek();
    int collisionFlag;
    std::unique_ptr<Obj3D> obj;
    if (next != std::char_traits<char>::eof() &&
        std::isalpha(static_cast<unsigned char>(next))) {
        std::string filename;
        if (!(in >> filename >> collisionFlag) || !endOfRecord(in)) return nullptr;

        std::string path = (std::filesystem::path(baseDir) / filename).string();
        std::ifstream animation(path);
        if (!animation) {
            fprintf(stderr, "ERROR: could not open animation file %s\n", path.c_str());
            return nullptr;
        }
        auto animated = std::make_unique<AnimatedObj3D>();
        if (!readAnimation(animation, animated->transforms)) {
            fprintf(stderr, "ERROR: empty or malformed animation file %s\n", path.c_str());
            return nullptr;
        }
        obj = std::move(animated);
    } else {
        auto staticObj = std::make_unique<Obj3D>();
        if (!readTransform(in, staticObj->transform) || !(in >> collisionFlag) ||
            !endOfRecord(in)) return nullptr;
        obj = std::move(staticObj);
    }
    obj->reflect = collisionFlag != 0;
    return obj;
}
