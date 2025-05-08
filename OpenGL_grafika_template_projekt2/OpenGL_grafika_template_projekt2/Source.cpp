#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// Structure to represent a control point
struct ControlPoint {
    float x, y;
    bool isSelected = false;
};

std::vector<ControlPoint> controlPoints;
int pointDiameter = 5; // Default diameter for control points
bool isDragging = false;
int selectedPointIndex = -1;

GLuint shaderProgram;
GLuint VAO, VBO;

// Load shader from file
std::string loadShaderFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filename << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Function to compile shaders
GLuint compileShader(GLenum type, const std::string& source) {
    const char* shaderSource = source.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);

    // Check for compile errors
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation error:\n" << infoLog << std::endl;
    }
    return shader;
}

// Function to create shader program from files
void createShaderProgram() {
    std::string vertexCode = loadShaderFromFile("vertexShader.glsl");
    std::string fragmentCode = loadShaderFromFile("fragmentShader.glsl");

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader program linking error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

// Function to draw a circle (for control points)
void drawCircle(float cx, float cy, float r, int num_segments) {
    std::vector<float> vertices;
    for (int i = 0; i < num_segments; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);
        float x = r * cosf(theta);
        float y = r * sinf(theta);
        vertices.push_back(x + cx);
        vertices.push_back(y + cy);
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, num_segments);
}

// Function to calculate a point on the Bézier curve
ControlPoint calculateBezierPoint(float t, const std::vector<ControlPoint>& points) {
    std::vector<ControlPoint> temp = points;
    int n = temp.size() - 1;

    for (int k = 1; k <= n; k++) {
        for (int i = 0; i <= n - k; i++) {
            temp[i].x = (1 - t) * temp[i].x + t * temp[i + 1].x;
            temp[i].y = (1 - t) * temp[i].y + t * temp[i + 1].y;
        }
    }

    return temp[0];
}

// Function to draw the Bézier curve
void drawBezierCurve() {
    if (controlPoints.size() < 2) return;

    std::vector<float> curveVertices;
    const int segments = 100;

    for (int i = 0; i <= segments; i++) {
        float t = static_cast<float>(i) / segments;
        ControlPoint p = calculateBezierPoint(t, controlPoints);
        curveVertices.push_back(p.x);
        curveVertices.push_back(p.y);
    }

    glUseProgram(shaderProgram);
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // White color for curve

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, curveVertices.size() * sizeof(float), curveVertices.data(), GL_STATIC_DRAW);
    glDrawArrays(GL_LINE_STRIP, 0, curveVertices.size() / 2);
}

// Function to draw control lines
void drawControlLines() {
    if (controlPoints.size() < 2) return;

    std::vector<float> lineVertices;
    for (const auto& p : controlPoints) {
        lineVertices.push_back(p.x);
        lineVertices.push_back(p.y);
    }

    glUseProgram(shaderProgram);
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc, 0.5f, 0.5f, 0.5f); // Gray color for control lines

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_STATIC_DRAW);
    glDrawArrays(GL_LINE_STRIP, 0, lineVertices.size() / 2);
}

// Function to draw control points
void drawControlPoints() {
    glUseProgram(shaderProgram);
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc, 1.0f, 0.0f, 0.0f); // Red color for control points

    for (const auto& p : controlPoints) {
        drawCircle(p.x, p.y, pointDiameter / 800.0f, 32);
    }
}

// Mouse callback function
void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // Convert from screen coordinates to normalized device coordinates
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float x = (float)xpos / width * 2.0f - 1.0f;
    float y = 1.0f - (float)ypos / height * 2.0f;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Check if clicked on an existing control point
        bool pointSelected = false;
        for (size_t i = 0; i < controlPoints.size(); i++) {
            float dx = controlPoints[i].x - x;
            float dy = controlPoints[i].y - y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < pointDiameter / 800.0f) {
                selectedPointIndex = i;
                controlPoints[i].isSelected = true;
                isDragging = true;
                pointSelected = true;
                break;
            }
        }

        // If not clicked on existing point, add a new one
        if (!pointSelected) {
            ControlPoint newPoint;
            newPoint.x = x;
            newPoint.y = y;
            controlPoints.push_back(newPoint);
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        isDragging = false;
        if (selectedPointIndex != -1) {
            controlPoints[selectedPointIndex].isSelected = false;
            selectedPointIndex = -1;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        // Remove control point if clicked on one
        for (size_t i = 0; i < controlPoints.size(); i++) {
            float dx = controlPoints[i].x - x;
            float dy = controlPoints[i].y - y;
            float distance = sqrt(dx * dx + dy * dy);
            if (distance < pointDiameter / 800.0f) {
                controlPoints.erase(controlPoints.begin() + i);
                break;
            }
        }
    }
}

// Cursor position callback for dragging
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (isDragging && selectedPointIndex != -1) {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        float x = (float)xpos / width * 2.0f - 1.0f;
        float y = 1.0f - (float)ypos / height * 2.0f;

        controlPoints[selectedPointIndex].x = x;
        controlPoints[selectedPointIndex].y = y;
    }
}

// Key callback for changing point diameter
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_UP && pointDiameter < 9) {
            pointDiameter++;
        }
        else if (key == GLFW_KEY_DOWN && pointDiameter > 3) {
            pointDiameter--;
        }
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(800, 600, "KosaBalint TT88AW", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Set callbacks
    glfwSetMouseButtonCallback(window, mouseCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetKeyCallback(window, keyCallback);

    // Create shader program
    createShaderProgram();

    // Set up VAO and VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the Bézier curve
        drawBezierCurve();

        // Draw control lines
        drawControlLines();

        // Draw control points
        drawControlPoints();

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}