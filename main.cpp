#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <sstream>
#include <limits>
#include "Main.hpp"
#include "Obj3D.hpp"
#include "Projectile.hpp"

void resize(GLFWwindow *window, int width, int height) {
    glViewport(0,0, width, height);
}

void logError(int code, const char *description) {
    fprintf(stderr, "Glfw error code %d: %s\n", code, description);
}

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 320.0f;
float lastY = 240.0f;
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

    float sensitivity = 0.05f;
    yaw += xoffset * sensitivity;
    pitch += yoffset * sensitivity;

    if (pitch > 89.0f) {
        pitch = 89.0f;
    } else if (pitch < -89.0f) {
        pitch = -89.0f;
    }
}

char *readEntireFile(const char *fileName) {
    FILE *f = fopen(fileName, "r");
    if (!f) {
        fprintf(stderr, "ERROR: could not open shader file %s\n", fileName);
        return nullptr;
    }
    fseek(f, 0, SEEK_END);
    size_t fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *fileContents = (char *)calloc(fileSize + 1, 1);
    fread(fileContents, sizeof(char), fileSize, f);
    fclose(f);
    return fileContents;
}


Mesh *readObj(std::string filename) {
    Mesh *mesh = new Mesh;
    mesh->min = glm::vec3(std::numeric_limits<float>::infinity());
    mesh->max = glm::vec3(-std::numeric_limits<float>::infinity());
    Group *g_atual = new Group;
    bool primeiroGrupo = true;
    std::ifstream arq(filename);
    std::string line;
    while(getline(arq, line)) {
        std::stringstream sline;
        sline << line;
        std::string temp;
        sline >> temp;
        if(temp == "v"){
            // ler vértice ...
            float x, y, z;
            sline >> x >> y >> z;
            mesh->vertex.push_back(new glm::vec3(x, y, z));
            if (x < mesh->min.x) mesh->min.x = x;
            if (y < mesh->min.y) mesh->min.y = y;
            if (z < mesh->min.z) mesh->min.z = z;
            if (x > mesh->max.x) mesh->max.x = x;
            if (y > mesh->max.y) mesh->max.y = y;
            if (z > mesh->max.z) mesh->max.z = z;
        } else if (temp == "f") {
            // implementar lógica de variações
            // para face: v, v/t/n, v/t e v//n
            // while enquanto tem tokens em sline:
            Face *f = new Face;
            std::string token;
            while (sline >> token) { // v/t/n, por exemplo
                std::stringstream stoken;
                stoken << token;
                std::string aux;
                // v ou v//n (sem t) -> primeiro campo sempre presente
                getline(stoken, aux, '/');
                int v = atoi(aux.c_str()) - 1;
                f->verts.push_back(v);
                // campo do texto pode estar vazio
                if (stoken.peek() == '/') {
                    stoken.get(); // consome o '/'
                    getline(stoken, aux, '/');
                    if (!aux.empty()) {
                        f->texts.push_back(atoi(aux.c_str()) - 1);
                    } else {
                        f->texts.push_back(0);
                    }
                } else {
                    getline(stoken, aux, '/');
                    if (!aux.empty()) {
                        f->texts.push_back(atoi(aux.c_str()) - 1);
                    }
                }
                // campo da normal (pode não existir)
                if (stoken >> aux) {
                    if (!aux.empty()) {
                        f->norms.push_back(atoi(aux.c_str()) - 1);
                    } else {
                        f->norms.push_back(0);
                    }
                }
            }
            // triangularizar a face conforme a quantidade de vértices
            if (f->verts.size() == 3) {
                // três vértices: mantém a face atual
                g_atual->faces.push_back(f);
            } else if (f->verts.size() == 4) {
                // quatro vértices: dois triângulos 0-1-3 e 1-2-3
                int v[4], t[4], n[4];
                for (int i = 0; i < 4; i++) {
                    v[i] = f->verts[i];
                    t[i] = (i < (int)f->texts.size()) ? f->texts[i] : 0;
                    n[i] = (i < (int)f->norms.size()) ? f->norms[i] : 0;
                }
                delete f;
                Face *f1 = new Face;
                f1->push(v[0], t[0], n[0]);
                f1->push(v[1], t[1], n[1]);
                f1->push(v[3], t[3], n[3]);
                Face *f2 = new Face;
                f1->push(v[1], t[1], n[1]);
                f1->push(v[2], t[2], n[2]);
                f1->push(v[3], t[3], n[3]);
                g_atual->faces.push_back(f1);
            } else if (f->verts.size() > 4) {
                // polígono convexo: vértice central na posição média
                glm::vec3 center(0.0f, 0.0f, 0.0f);
                for (int i = 0; i < (int)f->verts.size(); i++) {
                    center += *mesh->vertex[f->verts[i]];
                }
                center /= (float)f->verts.size();
                int centerIdx = (int)mesh->vertex.size();
                mesh->vertex.push_back(new glm::vec3(center));

                int count = (int)f->verts.size();
                int *v = new int[count];
                int *t = new int[count];
                int *n = new int[count];
                for (int i = 0; i < count; i++) {
                    v[i] = f->verts[i];
                    t[i] = (i < (int)f->texts.size()) ? f->texts[i] : 0;
                    n[i] = (i < (int)f->norms.size()) ? f->norms[i] : 0;
                }
                delete f;

                for (int i = 0; i < count; i++) {
                    int next = (i + 1) % count;
                    Face *tris = new Face;
                    tris->push(v[i], t[i], n[i]);
                    tris->push(v[next], t[next], n[next]);
                    tris->push(centerIdx, 0, 0);
                    g_atual->faces.push_back(tris);
                }
                delete[] v;
                delete[] t;
                delete[] n;
            }
        } else if (temp == "g") {
            // Inicia um novo grupo
            if (!primeiroGrupo) {
                mesh->groups.push_back(g_atual);
                g_atual = new Group;
            }
            primeiroGrupo = false;
        } else if (temp == "vn") {
            // ler normal ...
            float x, y, z;
            sline >> x >> y >> z;
            // ... atribuir normais da malha
            mesh->normals.push_back(new glm::vec3(x, y, z));
        } else if (temp == "vt") {
            // ler texto ...
            float u, v;
            sline >> u >> v;
            // ... atribuir textos da malha
            mesh->texts.push_back(new glm::vec2(u, v));
        }
    }
    if (g_atual != nullptr) {
        mesh->groups.push_back(g_atual);
    }
    return mesh;
}

void loadConfig(const char *fileName, std::vector<Mesh*> &meshes, std::vector<Obj3D> &objects) {
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
                obj.mesh = meshes[meshIndex];
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
            obj.collision = collisionFlag != 0;
            objects.push_back(obj);
        }
    }
}

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(640, 480, "Teste de versão OpenGL", NULL, NULL);
    if (!window) {
        fprintf (stderr, "ERROR: could not open window with GLFW3\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    glfwSetWindowSizeCallback(window, resize);
    glfwSetErrorCallback(logError);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    const char *vertexShader = readEntireFile("vertex.vs");
    const char *fragmentShader = readEntireFile("fragment.fs");
    if (!vertexShader || !fragmentShader) {
        fprintf(stderr, "ERROR: could not load shaders\n");
        glfwTerminate();
        return 1;
    }

    // identifica vs e o associa com vertex_shader
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShader, NULL);
    glCompileShader(vs);
    // verifica sucesso da compilação do vertex shader
    GLint compiled = GL_FALSE;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        GLint length = 0;
        glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length > 0 ? length : 1);
        glGetShaderInfoLog(vs, (GLsizei)log.size(), NULL, log.data());
        fprintf(stderr, "ERROR: vertex shader compilation failed:\n%s\n", log.data());
        return 1;
    }
    // identifica fs e o associa com fragment_shader
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShader, NULL);
    glCompileShader(fs);
    // verifica sucesso da compilação do fragment shader
    glGetShaderiv(fs, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        GLint length = 0;
        glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length > 0 ? length : 1);
        glGetShaderInfoLog(fs, (GLsizei)log.size(), NULL, log.data());
        fprintf(stderr, "ERROR: fragment shader compilation failed:\n%s\n", log.data());
        return 1;
    }
    // identifica do programa, adiciona partes e faz "linkagem"
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, fs);
    glAttachShader(shaderProgram, vs);
    glLinkProgram(shaderProgram);
    // verifica sucesso da linkagem do programa
    GLint linked = GL_FALSE;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
        GLint length = 0;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length > 0 ? length : 1);
        glGetProgramInfoLog(shaderProgram, (GLsizei)log.size(), NULL, log.data());
        fprintf(stderr, "ERROR: program link failed:\n%s\n", log.data());
        return 1;
    }

    // obtenção de versão suportada da OpenGL e renderizador
    glUseProgram (shaderProgram);

    GLint uniView = glGetUniformLocation(shaderProgram, "view");
    GLint uniProjection = glGetUniformLocation(shaderProgram, "projection");
    GLint uniTransform = glGetUniformLocation(shaderProgram, "transform");

    // Matriz de projeção padrão (perspective)
    float aspect = 640.0f / 480.0f;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

    glUniformMatrix4fv(uniProjection, 1, GL_FALSE, &projection[0][0]);

    std::vector<Mesh*> meshes;
    std::vector<Obj3D> objects;
    std::vector<Projectile*> projectiles;
    loadConfig("config.cfg", meshes, objects);
    Mesh *sphereMesh = meshes.size() > 1 ? meshes[1] : nullptr;

    // Carrega os dados de cada malha para a GPU
    for (auto* mesh : meshes) {
        for (const auto& g : mesh->groups) {
            std::vector<float> vs;
            std::vector<float> vts;
            std::vector<float> vns;
            for (const auto& f: g->faces) {
                for (unsigned int i = 0; i < f->verts.size(); i++) {
                    glm::vec3 v = *mesh->vertex[f->verts[i]];
                    vs.push_back(v.x);
                    vs.push_back(v.y);
                    vs.push_back(v.z);

                    if (i < f->texts.size() && f->texts[i] >= 0 && f->texts[i] < (int)mesh->texts.size()) {
                        glm::vec2 vt = *mesh->texts[f->texts[i]];
                        vts.push_back(vt.x);
                        vts.push_back(vt.y);
                    } else {
                        vts.push_back(0.0f);
                        vts.push_back(0.0f);
                    }

                    if (i < f->norms.size() && f->norms[i] >= 0 && f->norms[i] < (int)mesh->normals.size()) {
                        glm::vec3 vn = *mesh->normals[f->norms[i]];
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
            g->numberOfVertices = vs.size() / 3;

            glGenVertexArrays(1, &g->VAO);
            glBindVertexArray(g->VAO);

            GLuint vbos[3];
            glGenBuffers(3, vbos);

            glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
            glBufferData(GL_ARRAY_BUFFER, vs.size() * sizeof(float), vs.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
            glBufferData(GL_ARRAY_BUFFER, vts.size() * sizeof(float), vts.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);

            glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
            glBufferData(GL_ARRAY_BUFFER, vns.size() * sizeof(float), vns.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(2);

            glBindVertexArray(0);
        }
    }

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    const float cameraSpeed = 0.05f;
    double lastTime = glfwGetTime();
    bool spacePressed = false;
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        // Processa entrada do teclado (WASD) para translação da câmera
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            cameraPos += cameraSpeed * cameraFront;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cameraPos -= cameraSpeed * cameraFront;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        }

        // Dispara uma esfera na posição da câmera na direção que ela aponta
        bool spaceIsPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spaceIsPressed && !spacePressed && sphereMesh != nullptr) {
            projectiles.push_back(new Projectile(sphereMesh, cameraPos, cameraFront));
        }
        spacePressed = spaceIsPressed;

        // Avalia a direção da câmera a partir dos ângulos de Euler (mouse)
        cameraFront = glm::normalize(glm::vec3(
            cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            sin(glm::radians(yaw)) * cos(glm::radians(pitch))));

        // Avalia a matriz de visão (lookAt) a cada frame
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(uniView, 1, GL_FALSE, &view[0][0]);

        // Atualiza posição e verifica expiração de cada projétil
        for (auto it = projectiles.begin(); it != projectiles.end(); ) {
            if ((*it)->step(deltaTime, objects)) {
                delete *it;
                it = projectiles.erase(it);
            } else {
                ++it;
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Para cada Obj3D, define a matriz transform e desenha todas as malhas
        for (const auto& obj : objects) {
            glUniformMatrix4fv(uniTransform, 1, GL_FALSE, &obj.transform[0][0]);
            for (const auto& g : obj.mesh->groups) {
                glBindVertexArray(g->VAO);
                glDrawArrays(GL_TRIANGLES, 0, g->numberOfVertices);
            }
        }
        // Desenha todas as esferas lançadas
        for (const auto& p : projectiles) {
            glUniformMatrix4fv(uniTransform, 1, GL_FALSE, &p->transform[0][0]);
            for (const auto& g : p->mesh->groups) {
                glBindVertexArray(g->VAO);
                glDrawArrays(GL_TRIANGLES, 0, g->numberOfVertices);
            }
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    for (auto* p : projectiles) {
        delete p;
    }
    // encerra contexto GL e outros recursos da GLFW
    glfwTerminate();
    return 0;
}
