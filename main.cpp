#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <fstream>
#include <sstream>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include "Obj3D.hpp"
#include "Projectile.hpp"
#include "stb_image.h"

// ---- Window / projection / camera constants ----
constexpr int   kWindowWidth    = 640;
constexpr int   kWindowHeight   = 480;
constexpr int   kGlVersionMajor = 4;
constexpr int   kGlVersionMinor = 1;
constexpr float kFov            = 45.0f;   // vertical field of view (degrees)
constexpr float kNearPlane      = 0.1f;
constexpr float kFarPlane       = 100.0f;
constexpr float kMouseSensitivity = 0.05f;
constexpr float kPitchLimit     = 89.0f;   // clamp to avoid gimbal flip
constexpr float kCameraSpeed    = 3.0f;    // units per second (framerate-independent)
constexpr int   kDecorationFlag = 2;       // config collision flag for draw-only objects

const glm::vec3 kClearColor(0.98f, 0.69f, 0.25f);

GLint g_uniProjection = -1;

void updateProjection(int width, int height) {
    if (width == 0 || height == 0 || g_uniProjection < 0) return;
    float aspect = (float)width / (float)height;
    glm::mat4 projection = glm::perspective(glm::radians(kFov), aspect, kNearPlane, kFarPlane);
    glUniformMatrix4fv(g_uniProjection, 1, GL_FALSE, &projection[0][0]);
}

void resize(GLFWwindow *window, int /*width*/, int /*height*/) {
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    updateProjection(fbWidth, fbHeight);
}

void logError(int code, const char *description) {
    fprintf(stderr, "Glfw error code %d: %s\n", code, description);
}

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = kWindowWidth / 2.0f;
float lastY = kWindowHeight / 2.0f;
bool firstMouse = true;

void mouseCallback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = (float)(xpos - lastX);
    float yoffset = (float)(lastY - ypos);
    lastX = xpos;
    lastY = ypos;

    yaw += xoffset * kMouseSensitivity;
    pitch += yoffset * kMouseSensitivity;

    if (pitch > kPitchLimit) {
        pitch = kPitchLimit;
    } else if (pitch < -kPitchLimit) {
        pitch = -kPitchLimit;
    }
}

// Reads a whole file into a string. Returns an empty string on failure.
std::string readFile(const char *fileName) {
    std::ifstream in(fileName);
    if (!in) {
        fprintf(stderr, "ERROR: could not open file %s\n", fileName);
        return std::string();
    }
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Compiles a shader of the given type; returns 0 (and logs) on failure.
GLuint compileShader(GLenum type, const char *src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length > 0 ? length : 1);
        glGetShaderInfoLog(shader, (GLsizei)log.size(), NULL, log.data());
        fprintf(stderr, "ERROR: shader compilation failed:\n%s\n", log.data());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// Links a vertex + fragment shader into a program; returns 0 (and logs) on failure.
GLuint linkProgram(GLuint vs, GLuint fs) {
    GLuint program = glCreateProgram();
    glAttachShader(program, fs);
    glAttachShader(program, vs);
    glLinkProgram(program);
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length > 0 ? length : 1);
        glGetProgramInfoLog(program, (GLsizei)log.size(), NULL, log.data());
        fprintf(stderr, "ERROR: program link failed:\n%s\n", log.data());
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

// Fetches a uniform location, warning once if the shader doesn't declare it.
// (glUniform* silently ignores a location of -1, so uses stay safe.)
GLint getUniform(GLuint program, const char *name) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc < 0) {
        fprintf(stderr, "WARNING: uniform '%s' not found in shader program\n", name);
    }
    return loc;
}

// Returns the directory portion of a path (with trailing slash), or "" if none.
std::string dirOf(const std::string &path) {
    size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) return "";
    return path.substr(0, slash + 1);
}

GLuint loadTexture(const std::string &filename) {
    stbi_set_flip_vertically_on_load(true);

    int width, height, channels;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    if (!data) {
        fprintf(stderr, "ERROR: could not load texture %s: %s\n",
                filename.c_str(), stbi_failure_reason());
        return 0;
    }
    GLenum format;
    if (channels == 1) {
        format = GL_RED;
    } else if (channels == 4) {
        format = GL_RGBA;
    } else {
        format = GL_RGB; 
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // rows may not be 4-byte aligned
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);
    return tex;
}

void readMtl(const std::string &filename, std::vector<Material> &materials) {
    std::ifstream in(filename);
    if (!in) {
        fprintf(stderr, "ERROR: could not open material file %s\n", filename.c_str());
        return;
    }
    Material cur;
    bool haveMaterial = false;
    std::string line;
    while (getline(in, line)) {
        std::stringstream sline(line);
        std::string tag;
        sline >> tag;
        if (tag == "newmtl") {
            if (haveMaterial) materials.push_back(cur);
            cur = Material();
            haveMaterial = true;
            sline >> cur.name;
        } else if (!haveMaterial) {
            continue;
        } else if (tag == "Ka") {
            sline >> cur.ambient.x >> cur.ambient.y >> cur.ambient.z;
        } else if (tag == "Kd") {
            sline >> cur.diffuse.x >> cur.diffuse.y >> cur.diffuse.z;
        } else if (tag == "Ks") {
            sline >> cur.specular.x >> cur.specular.y >> cur.specular.z;
        } else if (tag == "Ns") {
            sline >> cur.shininess;
        } else if (tag == "map_Kd") {
            sline >> cur.diffuseMap;
        }
    }
    if (haveMaterial) materials.push_back(cur);
}

std::unique_ptr<Mesh> readObj(const std::string &filename) {
    auto mesh = std::make_unique<Mesh>();
    mesh->min = glm::vec3(std::numeric_limits<float>::infinity());
    mesh->max = glm::vec3(-std::numeric_limits<float>::infinity());
    std::ifstream arq(filename);
    if (!arq) {
        fprintf(stderr, "ERROR: could not open obj file %s\n", filename.c_str());
        return mesh; // empty mesh keeps config mesh indices aligned
    }
    Group cur;
    bool firstGroup = true;
    std::string line;
    while (getline(arq, line)) {
        std::stringstream sline(line);
        std::string temp;
        sline >> temp;
        if (temp == "v") {
            // ler vértice ...
            float x, y, z;
            sline >> x >> y >> z;
            glm::vec3 v(x, y, z);
            mesh->vertex.push_back(v);
            mesh->min = glm::min(mesh->min, v);
            mesh->max = glm::max(mesh->max, v);
        } else if (temp == "f") {
            // Parse cada token (v, v/t/n, v/t, v//n) numa face.
            Face f;
            std::string token;
            while (sline >> token) {
                std::stringstream stoken(token);
                std::string aux;
                // v ou v//n (sem t) -> primeiro campo sempre presente
                getline(stoken, aux, '/');
                f.verts.push_back(atoi(aux.c_str()) - 1);
                // campo do texto pode estar vazio
                if (stoken.peek() == '/') {
                    stoken.get(); // consome o '/'
                    getline(stoken, aux, '/');
                    f.texts.push_back(aux.empty() ? 0 : atoi(aux.c_str()) - 1);
                } else {
                    getline(stoken, aux, '/');
                    if (!aux.empty()) {
                        f.texts.push_back(atoi(aux.c_str()) - 1);
                    }
                }
                // campo da normal (pode não existir)
                if (stoken >> aux) {
                    f.norms.push_back(aux.empty() ? 0 : atoi(aux.c_str()) - 1);
                }
            }
            // triangularizar a face conforme a quantidade de vértices
            if (f.verts.size() == 3) {
                cur.faces.push_back(f);
            } else if (f.verts.size() == 4) {
                // quatro vértices: dois triângulos 0-1-3 e 1-2-3
                int v[4], t[4], n[4];
                for (int i = 0; i < 4; i++) {
                    v[i] = f.verts[i];
                    t[i] = (i < (int)f.texts.size()) ? f.texts[i] : 0;
                    n[i] = (i < (int)f.norms.size()) ? f.norms[i] : 0;
                }
                Face f1;
                f1.push(v[0], t[0], n[0]);
                f1.push(v[1], t[1], n[1]);
                f1.push(v[3], t[3], n[3]);
                Face f2;
                f2.push(v[1], t[1], n[1]);
                f2.push(v[2], t[2], n[2]);
                f2.push(v[3], t[3], n[3]);
                cur.faces.push_back(f1);
                cur.faces.push_back(f2);
            } else if (f.verts.size() > 4) {
                // polígono convexo: triângulo-leque a partir do centróide
                glm::vec3 center(0.0f);
                for (size_t i = 0; i < f.verts.size(); i++) {
                    center += mesh->vertex[f.verts[i]];
                }
                center /= (float)f.verts.size();
                int centerIdx = (int)mesh->vertex.size();
                mesh->vertex.push_back(center);

                int count = (int)f.verts.size();
                std::vector<int> v(count), t(count), n(count);
                for (int i = 0; i < count; i++) {
                    v[i] = f.verts[i];
                    t[i] = (i < (int)f.texts.size()) ? f.texts[i] : 0;
                    n[i] = (i < (int)f.norms.size()) ? f.norms[i] : 0;
                }
                for (int i = 0; i < count; i++) {
                    int next = (i + 1) % count;
                    Face parte;
                    parte.push(v[i], t[i], n[i]);
                    parte.push(v[next], t[next], n[next]);
                    parte.push(centerIdx, 0, 0);
                    cur.faces.push_back(parte);
                }
            }
        } else if (temp == "g") {
            // Inicia um novo grupo
            if (!firstGroup) {
                mesh->groups.push_back(std::move(cur));
                cur = Group();
            }
            firstGroup = false;
        } else if (temp == "vn") {
            float x, y, z;
            sline >> x >> y >> z;
            mesh->normals.push_back(glm::vec3(x, y, z));
        } else if (temp == "vt") {
            float u, v;
            sline >> u >> v;
            mesh->texts.push_back(glm::vec2(u, v));
        } else if (temp == "mtllib") {
            // biblioteca de materiais: carrega o .mtl referenciado (mesmo diretório do .obj)
            std::string mtlName;
            sline >> mtlName;
            // Store the mtllib path including the .obj's directory so texture
            // paths later resolve relative to that directory (see loadTexture call).
            mesh->mtllib = dirOf(filename) + mtlName;
            readMtl(mesh->mtllib, mesh->materials);
        } else if (temp == "usemtl") {
            // Start a new group for the new material (mirrors 'g' behaviour).
            std::string matName;
            sline >> matName;
            if (!cur.faces.empty() && cur.material != matName) {
                mesh->groups.push_back(std::move(cur));
                cur = Group();
                firstGroup = false;
            }
            cur.material = matName;
        }
    }
    mesh->groups.push_back(std::move(cur));
    return mesh;
}

void loadConfig(const char *fileName, std::vector<std::unique_ptr<Mesh>> &meshes,
                std::vector<Obj3D> &objects) {
    std::ifstream in(fileName);
    if (!in) {
        fprintf(stderr, "ERROR: could not open config file %s\n", fileName);
        return;
    }
    std::string line;
    bool meshSection = true;
    while (getline(in, line)) {
        size_t start = line.find_first_not_of(" \t\r");
        if (start == std::string::npos) {
            if (meshSection) {
                meshSection = false;
            }
            continue;
        }
        if (line[start] == '#') {
            continue;
        }
        std::stringstream sline(line.substr(start));
        if (meshSection) {
            int index;
            std::string path;
            sline >> index >> path;
            (void)index;
            meshes.push_back(readObj(path));
        } else {
            int meshIndex;
            int collisionFlag;
            float tx, ty, tz, sx, sy, sz, rx, ry, rz;
            sline >> meshIndex >> tx >> ty >> tz >> sx >> sy >> sz >> rx >> ry >> rz >> collisionFlag;

            Obj3D obj;
            if (meshIndex >= 0 && meshIndex < (int)meshes.size()) {
                obj.mesh = meshes[meshIndex].get();
            } else {
                fprintf(stderr, "ERROR: config references invalid mesh index %d\n", meshIndex);
                continue;
            }
            glm::mat4 rotation = glm::mat4(1.0f);
            rotation = glm::rotate(rotation, glm::radians(rx), glm::vec3(1.0f, 0.0f, 0.0f));
            rotation = glm::rotate(rotation, glm::radians(ry), glm::vec3(0.0f, 1.0f, 0.0f));
            rotation = glm::rotate(rotation, glm::radians(rz), glm::vec3(0.0f, 0.0f, 1.0f));
            obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(tx, ty, tz)) * rotation *
                            glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, sz));
            obj.reflect = collisionFlag != 0;
            obj.init();
            objects.push_back(obj);
        }
    }
}

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, kGlVersionMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, kGlVersionMinor);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(kWindowWidth, kWindowHeight, "Teste de versão OpenGL", NULL, NULL);
    if (!window) {
        fprintf (stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    glfwSetWindowSizeCallback(window, resize);
    glfwSetFramebufferSizeCallback(window, resize);
    glfwSetErrorCallback(logError);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "ERROR: glewInit failed\n");
        glfwTerminate();
        return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    std::string vertexSrc = readFile("vertex.vs");
    std::string fragmentSrc = readFile("fragment.fs");
    if (vertexSrc.empty() || fragmentSrc.empty()) {
        fprintf(stderr, "ERROR: could not load shaders\n");
        glfwTerminate();
        return 1;
    }

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc.c_str());
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc.c_str());
    if (vs == 0 || fs == 0) {
        glfwTerminate();
        return 1;
    }
    GLuint shaderProgram = linkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (shaderProgram == 0) {
        glfwTerminate();
        return 1;
    }
    glUseProgram(shaderProgram);

    GLint uniView       = getUniform(shaderProgram, "view");
    GLint uniProjection = getUniform(shaderProgram, "projection");
    GLint uniTransform  = getUniform(shaderProgram, "transform");
    g_uniProjection = uniProjection;

    // Uniformes de material (Phong) e iluminação
    GLint uniMatAmbient   = getUniform(shaderProgram, "matAmbient");
    GLint uniMatDiffuse   = getUniform(shaderProgram, "matDiffuse");
    GLint uniMatSpecular  = getUniform(shaderProgram, "matSpecular");
    GLint uniMatShininess = getUniform(shaderProgram, "matShininess");
    GLint uniDiffuseTex   = getUniform(shaderProgram, "diffuseTexture");
    GLint uniLightDir     = getUniform(shaderProgram, "lightDir");
    GLint uniLightColor   = getUniform(shaderProgram, "lightColor");
    GLint uniViewPos      = getUniform(shaderProgram, "viewPos");

    // Textura branca 1x1 usada quando um material não tem map_Kd,
    // para que a multiplicação pela textura no shader seja neutra.
    GLuint whiteTexture;
    glGenTextures(1, &whiteTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    unsigned char whitePixel[3] = {255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // O sampler usa sempre a unidade de textura 0.
    glUniform1i(uniDiffuseTex, 0);

    // Initial projection matrix from actual framebuffer size
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    updateProjection(fbWidth, fbHeight);

    std::vector<std::unique_ptr<Mesh>> meshes;
    std::vector<Obj3D> objects;
    std::vector<std::unique_ptr<Projectile>> projectiles;
    loadConfig("config.cfg", meshes, objects);
    Mesh *sphereMesh = meshes.size() > 1 ? meshes[1].get() : nullptr;

    for (auto &mesh : meshes) {
        std::string baseDir = dirOf(mesh->mtllib);
        for (auto &mat : mesh->materials) {
            if (!mat.diffuseMap.empty()) {
                mat.textureID = loadTexture(baseDir + mat.diffuseMap);
            }
        }
    }

    // Carrega os dados de cada malha para a GPU
    for (auto &mesh : meshes) {
        for (auto &g : mesh->groups) {
            std::vector<float> vs;
            std::vector<float> vts;
            std::vector<float> vns;
            for (const auto &f : g.faces) {
                for (unsigned int i = 0; i < f.verts.size(); i++) {
                    glm::vec3 v = mesh->vertex[f.verts[i]];
                    vs.push_back(v.x);
                    vs.push_back(v.y);
                    vs.push_back(v.z);

                    if (i < f.texts.size() && f.texts[i] >= 0 && f.texts[i] < (int)mesh->texts.size()) {
                        glm::vec2 vt = mesh->texts[f.texts[i]];
                        vts.push_back(vt.x);
                        vts.push_back(vt.y);
                    } else {
                        vts.push_back(0.0f);
                        vts.push_back(0.0f);
                    }

                    if (i < f.norms.size() && f.norms[i] >= 0 && f.norms[i] < (int)mesh->normals.size()) {
                        glm::vec3 vn = mesh->normals[f.norms[i]];
                        vns.push_back(vn.x);
                        vns.push_back(vn.y);
                        vns.push_back(vn.z);
                    } else {
                        vns.push_back(0.0f);
                        vns.push_back(0.0f);
                        vns.push_back(0.0f);
                    }
                }
            }
            g.numberOfVertices = vs.size() / 3;

            glGenVertexArrays(1, &g.VAO);
            glBindVertexArray(g.VAO);

            glGenBuffers(3, g.VBOs);

            glBindBuffer(GL_ARRAY_BUFFER, g.VBOs[0]);
            glBufferData(GL_ARRAY_BUFFER, vs.size() * sizeof(float), vs.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ARRAY_BUFFER, g.VBOs[1]);
            glBufferData(GL_ARRAY_BUFFER, vts.size() * sizeof(float), vts.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);

            glBindBuffer(GL_ARRAY_BUFFER, g.VBOs[2]);
            glBufferData(GL_ARRAY_BUFFER, vns.size() * sizeof(float), vns.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(2);

            glBindVertexArray(0);
        }
    }

    glClearColor(kClearColor.r, kClearColor.g, kClearColor.b, 1.0f);

    Material defaultMaterial;
    defaultMaterial.ambient  = glm::vec3(0.2f, 0.2f, 0.2f);
    defaultMaterial.diffuse  = glm::vec3(0.6f, 0.6f, 0.6f);
    defaultMaterial.specular = glm::vec3(0.3f, 0.3f, 0.3f);
    defaultMaterial.shininess = 16.0f;

    // Resolve o material de um grupo (por nome) e envia suas propriedades Phong
    // ao shader, ligando a textura difusa (ou a textura branca padrão).
    auto applyMaterial = [&](const Mesh *mesh, const Group &g) {
        const Material *mat = &defaultMaterial;
        for (const auto &m : mesh->materials) {
            if (m.name == g.material) { mat = &m; break; }
        }
        glActiveTexture(GL_TEXTURE0);
        glUniform3fv(uniMatAmbient, 1, &mat->ambient[0]);
        glUniform3fv(uniMatDiffuse, 1, &mat->diffuse[0]);
        glUniform3fv(uniMatSpecular, 1, &mat->specular[0]);
        glUniform1f(uniMatShininess, mat->shininess);
        GLuint tid = mat->textureID != 0 ? mat->textureID : whiteTexture;
        glBindTexture(GL_TEXTURE_2D, tid);
    };

    // Define a matriz transform de um objeto e desenha todas as suas malhas.
    auto drawObject = [&](const Obj3D &obj) {
        glUniformMatrix4fv(uniTransform, 1, GL_FALSE, &obj.transform[0][0]);
        for (const auto &g : obj.mesh->groups) {
            applyMaterial(obj.mesh, g);
            glBindVertexArray(g.VAO);
            glDrawArrays(GL_TRIANGLES, 0, g.numberOfVertices);
        }
    };

    const glm::vec3 skyDir = glm::normalize(glm::vec3(
        std::sin(glm::radians(60.0f)), std::cos(glm::radians(60.0f)), 0.0f));
    glUniform3fv(uniLightDir, 1, &skyDir[0]);
    glUniform3fv(uniLightColor, 1, &kClearColor[0]);

    double lastTime = glfwGetTime();
    bool spacePressed = false;
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        // Processa entrada do teclado (WASD) para translação da câmera
        float velocity = kCameraSpeed * deltaTime;
        glm::vec3 flatFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
        glm::vec3 flatRight = glm::normalize(glm::cross(flatFront, cameraUp));
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            cameraPos += velocity * flatFront;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cameraPos -= velocity * flatFront;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cameraPos -= flatRight * velocity;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cameraPos += flatRight * velocity;
        }

        // Dispara uma esfera na posição da câmera na direção que ela aponta
        bool spaceIsPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spaceIsPressed && !spacePressed && sphereMesh != nullptr) {
            projectiles.push_back(std::make_unique<Projectile>(sphereMesh, cameraPos, cameraFront));
        }
        spacePressed = spaceIsPressed;

        // Avalia a direção da câmera a partir dos ângulos de Euler (mouse)
        cameraFront = glm::normalize(glm::vec3(
            std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch)),
            std::sin(glm::radians(pitch)),
            std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch))));

        // Avalia a matriz de visão (lookAt) a cada frame
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(uniView, 1, GL_FALSE, &view[0][0]);
        glUniform3f(uniViewPos, cameraPos.x, cameraPos.y, cameraPos.z);

        // Atualiza posição e verifica expiração de cada projétil
        for (auto it = projectiles.begin(); it != projectiles.end(); ) {
            if ((*it)->step(deltaTime, objects)) {
                it = projectiles.erase(it);
            } else {
                ++it;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (const auto &obj : objects) {
            drawObject(obj);
        }
        for (const auto &p : projectiles) {
            drawObject(*p);
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Libera os recursos de GPU antes de encerrar.
    for (auto &mesh : meshes) {
        for (auto &g : mesh->groups) {
            glDeleteVertexArrays(1, &g.VAO);
            glDeleteBuffers(3, g.VBOs);
        }
        for (auto &mat : mesh->materials) {
            if (mat.textureID != 0) glDeleteTextures(1, &mat.textureID);
        }
    }
    glDeleteTextures(1, &whiteTexture);
    glDeleteProgram(shaderProgram);
    // encerra contexto GL e outros recursos da GLFW
    glfwTerminate();
    return 0;
}
