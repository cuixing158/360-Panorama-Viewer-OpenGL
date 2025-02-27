#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Sphere.h"

#define USE_GL_BEGIN_END 0

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

cv::Mat panoramaImage;

float pitch = 0.0f, yaw = 0.0f, prevPitch = 0.0f;  // 摄像机旋转角度
float fov = 90.0f;                                 // 初始视野角度
bool isDragging = false;                           // 是否正在拖动鼠标
double lastX = 0.0, lastY = 0.0;                   // 上次鼠标的位置

enum ViewMode { CENTER_VIEW,
                TINY_PLANET,
                CRYSTAL_BALL };
ViewMode currentView = CENTER_VIEW;

SphereData* sphere = nullptr;  // SphereData 实例

GLuint vao, vboVertices, vboIndices, vboTexCoords;  // 顶点数组对象和缓冲对象
GLuint shaderProgram, texture;                      // 着色器程序和纹理对象

const char* vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    uniform mat4 projection;
    uniform mat4 view;
    void main() {
        TexCoord = aTexCoord;
        gl_Position = projection * view * vec4(aPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D texture1;
    void main() {
        FragColor = texture(texture1, TexCoord);
    }
)";

// 鼠标按下和移动回调函数
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (isDragging) {
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;  // Y轴是反向的
        lastX = xpos;
        lastY = ypos;

        // 更新相机角度（偏航yaw和俯仰pitch）
        float sensitivity = 0.2f;  // 鼠标灵敏度
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        // 限制俯仰角度在[-89, 89]范围内，避免摄像机翻转
        // pitch = glm::clamp(pitch, -89.0f, 89.0f);
    }
}

// 鼠标按下回调函数
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isDragging = true;
            glfwGetCursorPos(window, &lastX, &lastY);  // 记录鼠标按下时的位置
        }
        if (action == GLFW_RELEASE) {
            isDragging = false;  // 释放鼠标时停止拖动
        }
    }
}

// 滚轮回调函数（用于调整 FOV）
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    fov -= 4.0f * (float)yoffset;         // 鼠标滚轮垂直移动
    fov = glm::clamp(fov, 1.0f, 160.0f);  // 限制 FOV 的范围
}

// Function to create a shader program
GLuint createProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    // 检查顶点着色器编译是否成功
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
        return 0;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    // 检查片段着色器编译是否成功
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // 检查程序链接是否成功
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void initPanoramaRenderer(SphereData* sphereData) {
    // 创建着色器程序
    shaderProgram = createProgram(vertexShaderSource, fragmentShaderSource);

    // 生成 VAO 和 VBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboVertices);
    glGenBuffers(1, &vboIndices);
    glGenBuffers(1, &vboTexCoords);

    // 绑定 VAO
    glBindVertexArray(vao);

    // 顶点数据
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
    glBufferData(GL_ARRAY_BUFFER, sphereData->getNumVertices() * sizeof(GLfloat), sphereData->getVertices(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    // 纹理坐标数据
    glBindBuffer(GL_ARRAY_BUFFER, vboTexCoords);
    glBufferData(GL_ARRAY_BUFFER, sphereData->getNumTexs() * sizeof(GLfloat), sphereData->getTexCoords(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    // 索引数据
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereData->getNumIndices() * sizeof(GLushort), sphereData->getIndices(), GL_STATIC_DRAW);

    // 解绑 VAO
    glBindVertexArray(0);
}

// 加载全景图像
GLuint loadTexture(const char* path) {
    cv::Mat image = cv::imread(path, cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "无法加载图像: " << path << std::endl;
        exit(1);
    }

    std::cout << "Loaded image with size: " << image.cols << "x" << image.rows << std::endl;

    cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
    cv::flip(image, image, 0);
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.cols, image.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return textureID;
}

// 绘制球体，该函数是传统的立即模式渲染函数glBegin/glEnd，现代OpenGL中不推荐使用
void renderSphere(float radius, int slices, int stacks) {
    for (int i = 0; i < stacks; ++i) {
        float phi0 = glm::pi<float>() * (-0.5 + (double)(i) / stacks);
        float phi1 = glm::pi<float>() * (-0.5 + (double)(i + 1) / stacks);

        glBegin(GL_TRIANGLE_STRIP);  // glBegin在OpenGL ES中不被支持，OpenGL ES 是 OpenGL 的一个子集，主要用于嵌入式系统
        for (int j = 0; j <= slices; ++j) {
            float theta = 2 * glm::pi<float>() * (double)(j) / slices;

            for (int k = 0; k < 2; ++k) {
                float phi = (k == 0) ? phi0 : phi1;
                float x = cos(phi) * cos(theta);
                float y = -sin(phi);
                float z = cos(phi) * sin(theta);

                glTexCoord2f((float)j / slices, 1.0f - (float)(i + k) / stacks);
                glVertex3f(radius * x, radius * y, radius * z);
            }
        }
        glEnd();
    }
}

// 处理用户输入
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) pitch += 0.5f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) pitch -= 0.5f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) yaw -= 0.5f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) yaw += 0.5f;

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        currentView = CENTER_VIEW;
        pitch = 0.0f;
        prevPitch = 0.0f;
        yaw = 0.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        currentView = TINY_PLANET;
        pitch = 89.0f;
        prevPitch = pitch;
        yaw = 0.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        currentView = CRYSTAL_BALL;
        pitch = 0.0f;
        prevPitch = pitch;
        yaw = 0.0f;
    }

    // 只有透视图才限制俯仰角度
    if (currentView == CENTER_VIEW) {
        pitch = glm::clamp(pitch, -89.0f, 89.0f);
    }
    yaw = glm::mod(yaw, 360.0f);
}

bool hasDivisibleNode(float previousPitch, float pitch) {
    // 确保 previousPitch 小于 pitch
    if (previousPitch > pitch) std::swap(previousPitch, pitch);

    // 不包括边界
    float lowerBound = previousPitch + std::numeric_limits<float>::epsilon();
    float upperBound = pitch - std::numeric_limits<float>::epsilon();

    // 找到第一个大于 lowerBound 的 "90 + 180*k" 的数
    float start = 90.0f + std::ceil((lowerBound - 90.0f) / 180.0f) * 180.0f;

    // 判断 start 是否在范围内
    return start > lowerBound && start < upperBound;
}

// 设置视图矩阵
void getViewMatrix(glm::mat4& projection, glm::mat4& view) {
    static glm::vec3 upCamera = glm::vec3(0.0f, 1.0f, 0.0f);
    // 设置投影矩阵
    projection = glm::perspective(glm::radians(fov), (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 100.0f);

    // 根据当前视角模式设置视图矩阵
    glm::vec3 movingPosition(sin(glm::radians(yaw)) * cos(glm::radians(pitch)), sin(glm::radians(pitch)), cos(glm::radians(yaw)) * cos(glm::radians(pitch)));  // 移动视角位置
    if (currentView == CENTER_VIEW) {
        glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        view = glm::lookAt(cameraPosition, movingPosition,
                           glm::vec3(0, 1, 0));
    } else if (currentView == TINY_PLANET) {
        glm::vec3 cameraPosition = movingPosition;  // 在单位球表面

        if (hasDivisibleNode(prevPitch, pitch)) {
            upCamera[1] = upCamera[1] * -1.0f;
        }

        view = glm::lookAt(cameraPosition, glm::vec3(0.0f, 0.0f, 0.0f), upCamera);
        prevPitch = pitch;
    } else if (currentView == CRYSTAL_BALL) {
        glm::vec3 cameraPosition = 1.5f * movingPosition;  // 球外部
        if (hasDivisibleNode(prevPitch, pitch)) {
            upCamera[1] = upCamera[1] * -1.0f;
        }
        view = glm::lookAt(cameraPosition, glm::vec3(0.0f, 0.0f, 0.0f), upCamera);

        prevPitch = pitch;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(projection));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(view));
}

void renderPanorama(SphereData* sphereData, glm::mat4 projection, glm::mat4 view) {
    glUseProgram(shaderProgram);

    // 设置 uniform 矩阵
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // 绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    // 绘制球体
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, sphereData->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);

    glUseProgram(0);
}

// 渲染循环
void renderLoop(GLFWwindow* window, SphereData* sphereData) {
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        processInput(window);

        glm::mat4 projection, view;
        getViewMatrix(projection, view);  // 获取投影和视角矩阵

#if USE_GL_BEGIN_END
        renderSphere(1.0f, 50, 50);
#else
        renderPanorama(sphereData, projection, view);
#endif

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW 初始化失败!" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "360 Panorama Viewer", nullptr, nullptr);
    if (!window) {
        std::cerr << "窗口创建失败!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewInit();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    // 初始化 SphereData
    sphere = new SphereData(1.0f, 50, 50);

    initPanoramaRenderer(sphere);

    std::string imagePath = "../images/360panorama.jpg";
    texture = loadTexture(imagePath.c_str());

    glBindBuffer(GL_ARRAY_BUFFER, 0);  // 解绑 VBO,360全景图像最好需要
    glBindVertexArray(0);              // 解绑VAO,360全景图像最好需要
    glGenerateMipmap(GL_TEXTURE_2D);   //全景图像使用，但是视频渲染不使用 glGenerateMipmap,较少性能开销

    // 启用深度测试，防止遮挡影响
    glEnable(GL_DEPTH_TEST);
    // 设置深度测试函数
    glDepthFunc(GL_LESS);

    // 设置回调函数
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // 渲染循环
    renderLoop(window, sphere);

    // 清理资源
    delete sphere;

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
