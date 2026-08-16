#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>

#define HALF_COS60 0.25f
#define HALF_SIN60 0.433f

void resize(GLFWwindow *window, int width, int height) {
    glViewport(0,0, width, height);
}

void logError(int code, const char *description) {
    fprintf(stderr, "Glfw error code %d: %s\n", code, description);
}

char *readEntireFile(char *fileName) {
    FILE *f = fopen(fileName, "r");
    fseek(f, 0, SEEK_END);
    size_t fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *fileContents = calloc(fileSize + 1, 1);
    fread(fileContents, sizeof(char), fileSize, f);
    fclose(f);
    return fileContents;
}

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "ERROR: could not start GLFW3\n");
        return 1;
    }
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

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    GLfloat vertices[] = {
        // Positions       // Colors
        HALF_COS60, -HALF_SIN60, 0.0f, 1.0f, 0.0f, 1.0f, 
        -HALF_COS60, -HALF_SIN60, 0.0f, 0.0f, 0.0f, 1.0f, // Bottom
        0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                                           
        0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // Bottom Right
        HALF_COS60, -HALF_SIN60, 0.0f, 1.0f, 0.0f, 1.0f, 
        0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 
                                           
        HALF_COS60, HALF_SIN60, 0.0f, 1.0f, 1.0f, 0.0f, 
        0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // Top Right
        0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 
                                           
        -HALF_COS60, HALF_SIN60, 0.0f, 0.0f, 1.0f, 0.0f, 
        HALF_COS60, HALF_SIN60, 0.0f, 1.0f, 1.0f, 0.0f, // Top 
        0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 
                                           
        -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 
        -HALF_COS60, HALF_SIN60, 0.0f, 0.0f, 1.0f, 0.0f, // Top left
        0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 
                                           
        -HALF_COS60, -HALF_SIN60, 0.0f, 0.0f, 0.0f, 1.0f, // Bottom Left
        -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 
        0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // Center
        };

    const char *vertexShader = readEntireFile("hexagon.vs");
    const char *fragmentShader = readEntireFile("hexagon.fs");

    // identifica vs e o associa com vertex_shader
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShader, NULL);
    glCompileShader(vs);
    // identifica fs e o associa com fragment_shader
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShader, NULL);
    glCompileShader(fs);
    // identifica do programa, adiciona partes e faz "linkagem"
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, fs);
    glAttachShader(shaderProgram, vs);
    glLinkProgram(shaderProgram);

    // obtenção de versão suportada da OpenGL e renderizador
    glUseProgram (shaderProgram);

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // Posições:
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // Cores:
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Define vao como vertex array atual
        glBindVertexArray(VAO);
        // desenha pontos a partir do p0 e 3 no total do VAO atual com o shader
        // atualmente em uso
        for(int i = 0; i < 18; i+=3) {
            glDrawArrays(GL_TRIANGLES, i, 3);
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // encerra contexto GL e outros recursos da GLFW
    glfwTerminate();
    return 0;
}
