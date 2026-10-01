#pragma once
#include <string>

#include <GL/glew.h>
#include <glm/vec3.hpp>

// Phong material properties parsed from a Wavefront .mtl file.
class Material {
public:
    std::string name;
    glm::vec3 ambient  = glm::vec3(0.2f);  // Ka
    glm::vec3 diffuse  = glm::vec3(0.8f);  // Kd
    glm::vec3 specular = glm::vec3(0.0f);  // Ks
    float shininess    = 8.0f;             // Ns (specular exponent)
    std::string diffuseMap;                // map_Kd (texture file, relative to the .mtl)
    GLuint textureID = 0;                  // GL texture handle (0 = use default white texture)
};
