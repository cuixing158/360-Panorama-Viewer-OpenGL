#ifndef PANORAMARENDERER_H
#define PANORAMARENDERER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Sphere.h"

#define USE_GL_BEGIN_END 1

class PanoramaRenderer {
   public:
    enum class SwitchMode { PANORAMAVIDEO,
                            PANORAMAIMAGE };  //全景视频，全景图像
    enum class ViewMode { PERSPECTIVE,
                          LITTLEPLANET,
                          CRYSTALBALL };  // 透视图,小行星，水晶球视角看全景

    PanoramaRenderer(std::string filepath);
    // 渲染循环
    void renderLoop();
    // 析构函数
    ~PanoramaRenderer();

   private:
    bool isImageFile(const std::string &filepath);
    bool isVideoFile(const std::string &filepath);
    void updateVideoFrame();

    // Function to create a shader program
    GLuint createProgram(const char *vertexSource, const char *fragmentSource);

    void initPanoramaRenderer();

    // 加载全景图像
    GLuint loadTexture(const char *path);
    // 绘制球体，该函数是传统的立即模式渲染函数glBegin/glEnd，现代OpenGL中不推荐使用
    void renderSphere(float radius, int slices, int stacks);
    // 处理用户输入
    void processInput();
    bool hasDivisibleNode(float previousPitch, float pitch);
    // 获取视图矩阵
    void getViewMatrix(glm::mat4 &projection, glm::mat4 &view);
    void renderPanorama(SphereData *sphereData, glm::mat4 projection, glm::mat4 view);
    // 鼠标按下和移动回调函数
    void mouse_callback(double xpos, double ypos);
    // 鼠标按下回调函数
    void mouse_button_callback(int button, int action, int mods);
    // 滚轮回调函数（用于调整 FOV）
    void scroll_callback(double xoffset, double yoffset);

    GLFWwindow *window;
    // 全景图片和视频渲染
    GLuint vao, vboVertices, vboIndices, vboTexCoords;  // 顶点数组对象和缓冲对象
    GLuint shaderProgram, texture;                      // 着色器程序和纹理对象

    glm::mat4 projection;
    glm::mat4 view;

    ViewMode viewOrientation;  // 透视图，小行星，水晶球
    SwitchMode panoMode;       // 全景视频，全景图像切换

    // 播放屏幕宽和高尺寸
    int widthScreen;
    int heightScreen;

    float pitch, yaw, prevPitch;  // 摄像机旋转角度
    float fov;                    // 初始视野角度
    bool isDragging;              // 是否正在拖动鼠标
    double lastX, lastY;          // 上次鼠标的位置
    SphereData *sphereData;
    cv::VideoCapture videoCapture;
};

#endif  // PANORAMARENDERER_H
