#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

#define _USE_MATH_DEFINES
#include <math.h>

using namespace std;
using namespace glm;

const int WINDOW_SIZE = 600;
const float CIRCLE_RADIUS = 50.0f / (WINDOW_SIZE / 2.0f);
const float LINE_WIDTH = 3.0f / (WINDOW_SIZE / 2.0f);

GLuint circleShaderProgram, lineShaderProgram;
GLuint circleVAO, circleVBO, lineVAO, lineVBO;
GLfloat xPos = 0.0f, xSpeed = 0.00005f;
GLfloat yPos = 0.0f;
const GLfloat moveSpeed = 0.00005f;
int isIntersecting = 0;

string loadShaderFromFile(const string& filename) {
    ifstream file(filename);
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint loadShaders(const string& vertexPath, const string& fragmentPath) {
    string vertexCode = loadShaderFromFile(vertexPath);
    string fragmentCode = loadShaderFromFile(fragmentPath);

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vShaderCode = vertexCode.c_str();
    glShaderSource(vertexShader, 1, &vShaderCode, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fShaderCode = fragmentCode.c_str();
    glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
    glCompileShader(fragmentShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void initCircle() {
    vector<float> vertices;
    const int segments = 100;
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);

    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        vertices.push_back(cos(angle) * CIRCLE_RADIUS);
        vertices.push_back(sin(angle) * CIRCLE_RADIUS);
    }

    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);
    glBindVertexArray(circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void initLine() {
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, 2 * 2 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void renderCircle() {
    glUseProgram(circleShaderProgram);
    glUniform1i(glGetUniformLocation(circleShaderProgram, "isIntersecting"), isIntersecting);
    glUniform1f(glGetUniformLocation(circleShaderProgram, "xOffset"), xPos);
    glUniform2f(glGetUniformLocation(circleShaderProgram, "circleCenter"), xPos, 0.0f);
    glBindVertexArray(circleVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 102);
    glBindVertexArray(0);
}

void renderLine() {
    float lineVertices[] = { -0.33f, yPos, 0.33f, yPos };
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVertices), lineVertices);
    glUseProgram(lineShaderProgram);
    glBindVertexArray(lineVAO);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
}

void cleanup() {
    glDeleteVertexArrays(1, &circleVAO);
    glDeleteBuffers(1, &circleVBO);
    glDeleteVertexArrays(1, &lineVAO);
    glDeleteBuffers(1, &lineVBO);
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_SIZE, WINDOW_SIZE, "Kor es Vonal", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit();
    glViewport(0, 0, WINDOW_SIZE, WINDOW_SIZE);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    circleShaderProgram = loadShaders("vertexShader.glsl", "circleFragmentShader.glsl");
    lineShaderProgram = loadShaders("vertexShader.glsl", "lineFragmentShader.glsl");
    initCircle();
    initLine();

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Vonal mozgatása
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) yPos += moveSpeed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) yPos -= moveSpeed;

        // Metszés vizsgálat
        isIntersecting = (abs(yPos) <= CIRCLE_RADIUS &&
            (xPos + CIRCLE_RADIUS >= -0.33f && xPos - CIRCLE_RADIUS <= 0.33f)) ? 1 : 0;

        renderCircle();
        renderLine();

        // Kör mozgatása
        xPos += xSpeed;
        if (xPos + CIRCLE_RADIUS >= 1.0f || xPos - CIRCLE_RADIUS <= -1.0f) {
            xSpeed = -xSpeed; // Széléhez érve visszapattan a kör
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    cleanup();
    glDeleteProgram(circleShaderProgram);
    glDeleteProgram(lineShaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
