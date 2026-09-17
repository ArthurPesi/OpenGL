#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <fstream>
#include <sstream>
#include "Main.hpp"

void resize(GLFWwindow *window, int width, int height) {
    glViewport(0,0, width, height);
}

void logError(int code, const char *description) {
    fprintf(stderr, "Glfw error code %d: %s\n", code, description);
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

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    Mesh *m0 = new Mesh;
    glm::vec3 v0(-0.5f, -0.5f, 0.5f);
    glm::vec3 v1(-0.5f, 0.5f, 0.5f);
    glm::vec3 v2(0.5f, 0.5f, 0.5f);
    glm::vec3 v3(0.5f, -0.5f, 0.5f);

    glm::vec3 v4(-0.5f, -0.5f, -0.5f);
    glm::vec3 v5(-0.5f, 0.5f, -0.5f);
    glm::vec3 v6(0.5f, 0.5f, -0.5f);
    glm::vec3 v7(0.5f, -0.5f, -0.5f);

    m0->vertex.push_back(&v0);
    m0->vertex.push_back(&v1);
    m0->vertex.push_back(&v2);
    m0->vertex.push_back(&v3);

    m0->vertex.push_back(&v4);
    m0->vertex.push_back(&v5);
    m0->vertex.push_back(&v6);
    m0->vertex.push_back(&v7);

    glm::vec2 uv0(0.0f, 0.0f);
    glm::vec2 uv1(0.0f, 1.0f);
    glm::vec2 uv2(1.0f, 1.0f);
    glm::vec2 uv3(1.0f, 0.0f);

    m0->texts.push_back(&uv0);
    m0->texts.push_back(&uv1);
    m0->texts.push_back(&uv2);
    m0->texts.push_back(&uv3);

    glm::vec3 n0(0.0f, 0.0f, 1.0f);//front
    glm::vec3 n1(1.0f, 0.0f, 0.0f);//east
    glm::vec3 n2(0.0f, 0.0f, -1.0f);//back
    glm::vec3 n3(-1.0f, 0.0f, 0.0f);//west
    glm::vec3 n4(0.0f, 1.0f, 0.0f);//top
    glm::vec3 n5(0.0f, -1.0f, 0.0f);//down

    m0->normals.push_back(&n0);
    m0->normals.push_back(&n1);
    m0->normals.push_back(&n2);
    m0->normals.push_back(&n3);
    m0->normals.push_back(&n4);
    m0->normals.push_back(&n5);

    Group *g0 = new Group;
    m0->groups.push_back(g0);

    Face *faces = new Face[6];
    //front
    faces[0].push(0,0,0);
    faces[0].push(1,1,0);
    faces[0].push(3,3,0);

    faces[0].push(1,1,0);
    faces[0].push(2,2,0);
    faces[0].push(3,3,0);

    //east
    faces[1].push(2,0,1);
    faces[1].push(3,1,1);
    faces[1].push(7,3,1);

    faces[1].push(7,1,1);
    faces[1].push(6,2,1);
    faces[1].push(2,3,1);

    //back
    faces[2].push(7,0,2);
    faces[2].push(6,1,2);
    faces[2].push(4,3,2);

    faces[2].push(6,1,2);
    faces[2].push(5,2,2);
    faces[2].push(4,3,2);

    //West
    faces[3].push(4,0,3);
    faces[3].push(5,1,3);
    faces[3].push(0,3,3);

    faces[3].push(5,1,3);
    faces[3].push(1,2,3);
    faces[3].push(0,3,3);

    //Top
    faces[4].push(1,0,4);
    faces[4].push(2,1,4);
    faces[4].push(5,3,4);

    faces[4].push(5,1,4);
    faces[4].push(2,2,4);
    faces[4].push(6,3,4);

    //Bottom
    faces[5].push(4,0,5);
    faces[5].push(3,1,5);
    faces[5].push(0,3,5);

    faces[5].push(7,1,5);
    faces[5].push(3,2,5);
    faces[5].push(4,3,5);

    for(int i = 0; i < 6; i++) {
        g0->faces.push_back(&faces[i]);
    }

    //m0 = readObj("sphere.obj");

    for (const auto& g : m0->groups) {
        std::vector<float> vs;
        std::vector<float> vts;
        std::vector<float> vns;
        for (const auto& f: g->faces) {
            for (unsigned int i = 0; i < f->verts.size(); i++) {
                glm::vec3 v = *m0->vertex[f->verts[i]];
                vs.push_back(v.x);
                vs.push_back(v.y);
                vs.push_back(v.z);

                if (i < f->texts.size() && f->texts[i] >= 0 && f->texts[i] < (int)m0->texts.size()) {
                    glm::vec2 vt = *m0->texts[f->texts[i]];
                    vts.push_back(vt.x);
                    vts.push_back(vt.y);
                } else {
                    vts.push_back(0.0f);
                    vts.push_back(0.0f);
                }

                if (i < f->norms.size() && f->norms[i] >= 0 && f->norms[i] < (int)m0->normals.size()) {
                    glm::vec3 vn = *m0->normals[f->norms[i]];
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

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Define vao como vertex array atual e desenha os vértices de cada grupo
        for (const auto& g : m0->groups) {
            glBindVertexArray(g->VAO);
            glDrawArrays(GL_TRIANGLES, 0, g->numberOfVertices);
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // encerra contexto GL e outros recursos da GLFW
    glfwTerminate();
    return 0;
}
