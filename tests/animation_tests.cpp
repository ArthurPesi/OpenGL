#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "AnimatedObj3D.hpp"
#include "ObjectConfig.hpp"
#include "Projectile.hpp"

namespace {

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(const glm::mat4 &a, const glm::mat4 &b) {
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            if (std::abs(a[column][row] - b[column][row]) > 0.00001f) return false;
        }
    }
    return true;
}

bool near(const glm::vec3 &a, const glm::vec3 &b) {
    return glm::length(a - b) < 0.00001f;
}

std::unique_ptr<Obj3D> parseObject(const std::string &record, const std::string &baseDir) {
    std::istringstream in(record);
    return readObjectConfig(in, baseDir);
}

void testStaticConfig(const std::string &baseDir) {
    auto obj = parseObject(" -1.5 -0.5 -5.0 1 1 1 0 0 0 1", baseDir);
    require(obj != nullptr, "static transform must parse");
    require(dynamic_cast<AnimatedObj3D *>(obj.get()) == nullptr, "static object must stay static");
    require(obj->reflect, "static collision flag 1 must reflect");
    glm::mat4 expected = glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, -0.5f, -5.0f));
    require(near(obj->getTransform(), expected), "static transform changed");
    obj->updateAnimation(100.0);
    require(near(obj->getTransform(), expected), "static objects must not animate");

    obj = parseObject("\t+.5 -.5 .0 2 3 4 0 90 0 0 # destructible", baseDir);
    require(obj != nullptr && !obj->reflect, "signed and fractional static values must parse");
    expected = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -0.5f, 0.0f)) *
               glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0)) *
               glm::scale(glm::mat4(1.0f), glm::vec3(2, 3, 4));
    require(near(obj->getTransform(), expected), "static TRS order must be preserved");
    require(!parseObject("0 0 0 1 1 1 0 0 0", baseDir), "missing collision flag must fail");
    require(!parseObject("0 0 0 1 1 invalid 0 0 0 1", baseDir), "malformed static row must fail");
    require(!parseObject("", baseDir), "empty object record must fail");
}

void testAnimationRows() {
    std::istringstream in(
        "# animation\r\n\t\r\n"
        "4 5 6 2 3 4 30 45 60 # frame zero\r\n"
        "-1 -2 -3 1 1 1 0 0 0"); // Last frame has no trailing newline.
    std::vector<glm::mat4> frames;
    require(readAnimation(in, frames), "animation rows must parse through EOF");
    require(frames.size() == 2, "blank lines and comments must not create frames");
    glm::mat4 rotation(1.0f);
    rotation = glm::rotate(rotation, glm::radians(30.0f), glm::vec3(1, 0, 0));
    rotation = glm::rotate(rotation, glm::radians(45.0f), glm::vec3(0, 1, 0));
    rotation = glm::rotate(rotation, glm::radians(60.0f), glm::vec3(0, 0, 1));
    glm::mat4 expected = glm::translate(glm::mat4(1.0f), glm::vec3(4, 5, 6)) * rotation *
                         glm::scale(glm::mat4(1.0f), glm::vec3(2, 3, 4));
    require(near(frames[0], expected), "animation must use the same TRS order as static objects");
    require(near(frames[1], glm::translate(glm::mat4(1.0f), glm::vec3(-1, -2, -3))),
            "last frame must be stored in file order");

    for (const char *invalid : {"", "\n# no frames\n", "0 0 0 1 1 1 0 0",
                                "0 0 0 1 invalid 1 0 0 0", "0 0 0 1 1 1 0 0 0 1",
                                "0 0 0 1 1 1 0 0 0\n0 0 0"}) {
        std::istringstream bad(invalid);
        std::vector<glm::mat4> unchanged{glm::mat4(42.0f)};
        require(!readAnimation(bad, unchanged), "empty or malformed animation must fail");
        require(unchanged.size() == 1 && near(unchanged[0], glm::mat4(42.0f)),
                "failed animation parsing must not publish partial frames");
    }
}

void testSamplePlayback(const std::string &baseDir) {
    auto obj = parseObject(" \tcar.anim 1", baseDir);
    auto *animated = dynamic_cast<AnimatedObj3D *>(obj.get());
    require(animated != nullptr, "letter after mesh index must select AnimatedObj3D");
    require(animated->transforms.size() == 4, "car.anim must supply four frames");
    require(obj->reflect, "animated collision flag 1 must reflect");
    require(&obj->getTransform() == &animated->transforms[0], "playback must start at frame zero");
    for (std::size_t i = 0; i < animated->transforms.size(); ++i) {
        glm::mat4 expected = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.9f, 0.0f)) *
                             glm::rotate(glm::mat4(1.0f), glm::radians(90.0f * i), glm::vec3(0, 1, 0)) *
                             glm::scale(glm::mat4(1.0f), glm::vec3(0.12f));
        require(near(animated->transforms[i], expected), "car.anim frames must preserve file order");
    }

    const double times[] = {0.0, 0.499999, 0.5, 0.999999, 1.0, 1.5, 1.999999, 2.0, 2.5, 12345.5};
    const std::size_t indices[] = {0, 0, 1, 1, 2, 3, 3, 0, 1, 3};
    for (std::size_t i = 0; i < sizeof(times) / sizeof(times[0]); ++i) {
        obj->updateAnimation(times[i]);
        require(&obj->getTransform() == &animated->transforms[indices[i]],
                "2 Hz selection must advance every 0.5 seconds and loop with modulo");
    }

    obj = parseObject("car.anim 0 # destructible", baseDir);
    require(obj != nullptr && !obj->reflect, "animated collision flag 0 must be destructible");
    require(!parseObject("car.anim", baseDir), "animation record still requires a collision flag");
    require(!parseObject("missing-animation-test-file.anim 1", baseDir), "missing animation must fail");

    AnimatedObj3D single;
    std::istringstream oneFrame("1 2 3 1 1 1 0 0 0");
    require(readAnimation(oneFrame, single.transforms), "single-frame animation must parse");
    single.updateAnimation(999.5);
    require(&single.getTransform() == &single.transforms[0], "single-frame animation must loop safely");

    AnimatedObj3D empty;
    empty.updateAnimation(10.0);
    require(near(empty.getTransform(), glm::mat4(1.0f)), "empty vector must not cause modulo by zero");
}

void testCollisionBounds() {
    Mesh mesh;
    mesh.min = glm::vec3(-1, -2, -3);
    mesh.max = glm::vec3(1, 2, 3);
    auto animated = std::make_unique<AnimatedObj3D>();
    animated->mesh = &mesh;
    std::istringstream in("0 0 0 1 1 1 0 0 0\n10 0 0 1 1 1 0 90 0");
    require(readAnimation(in, animated->transforms), "collision test animation must parse");
    std::unique_ptr<Obj3D> obj = std::move(animated);
    obj->init();
    require(near(obj->worldMin, mesh.min) && near(obj->worldMax, mesh.max),
            "initial collision bounds must use frame zero");
    obj->updateAnimation(0.5);
    require(near(obj->worldMin, glm::vec3(7, -2, -1)) && near(obj->worldMax, glm::vec3(13, 2, 1)),
            "animation must refresh translated and rotated collision bounds");
    glm::vec3 normal, closest;
    require(obj->checkSphereCollision(glm::vec3(13.1f, 0, 0), 0.25f, normal, closest),
            "collision must detect the displayed pose");
    require(!obj->checkSphereCollision(glm::vec3(1.1f, 0, 0), 0.25f, normal, closest),
            "collision must no longer detect the old pose");
    obj->updateAnimation(1.0);
    require(near(obj->worldMin, mesh.min) && near(obj->worldMax, mesh.max),
            "looping must restore frame-zero collision bounds");
}

void testProjectileCollisions() {
    Mesh mesh;
    mesh.min = glm::vec3(-0.5f);
    mesh.max = glm::vec3(0.5f);
    for (bool reflect : {false, true}) {
        auto animated = std::make_unique<AnimatedObj3D>();
        animated->mesh = &mesh;
        animated->reflect = reflect;
        animated->transforms = {
            glm::translate(glm::mat4(1.0f), glm::vec3(5, 0, 0)), glm::mat4(1.0f)};
        animated->init();
        std::vector<std::unique_ptr<Obj3D>> objects;
        objects.push_back(std::move(animated));
        objects[0]->updateAnimation(0.5);
        Projectile projectile(&mesh, glm::vec3(-0.6f, 0, 0), glm::vec3(1, 0, 0));
        require(projectile.step(0.0f, objects) == !reflect, "collision flag must retain projectile behavior");
        require(objects.size() == (reflect ? 1u : 0u), "only destructible animated objects must be erased");
        if (reflect) {
            require(dynamic_cast<AnimatedObj3D *>(objects[0].get()) != nullptr,
                    "scene storage must preserve the animated subclass");
            projectile.step(0.01f, objects);
            require(projectile.getTransform()[3].x < -0.751f, "reflective collision must reverse the projectile");
        }
    }
}

void testSampleConfig(const std::string &baseDir) {
    std::ifstream in(baseDir + "/config.cfg");
    require(static_cast<bool>(in), "sample config must be readable");
    bool meshSection = true;
    std::size_t staticCount = 0, animatedCount = 0;
    std::string line;
    while (std::getline(in, line)) {
        std::size_t start = line.find_first_not_of(" \t\r");
        if (start == std::string::npos) {
            meshSection = false;
            continue;
        }
        if (meshSection || line[start] == '#') continue;
        std::istringstream record(line);
        int meshIndex;
        require(static_cast<bool>(record >> meshIndex), "sample mesh index must parse");
        auto obj = readObjectConfig(record, baseDir);
        require(obj != nullptr, "all sample object configurations must parse");
        if (dynamic_cast<AnimatedObj3D *>(obj.get())) {
            ++animatedCount;
            require(meshIndex == 7 && obj->reflect, "sample car mesh and collision flag must be preserved");
        } else {
            ++staticCount;
        }
    }
    require(staticCount == 16 && animatedCount == 1, "sample config must retain all static objects and its car");
}

} // namespace

int main(int argc, char **argv) {
    try {
        require(argc == 2, "pass the project root as the test argument");
        const std::string baseDir = argv[1];
        testStaticConfig(baseDir);
        testAnimationRows();
        testSamplePlayback(baseDir);
        testCollisionBounds();
        testProjectileCollisions();
        testSampleConfig(baseDir);
        printf("All animation checks passed (no window or OpenGL context created).\n");
        return 0;
    } catch (const std::exception &error) {
        fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
