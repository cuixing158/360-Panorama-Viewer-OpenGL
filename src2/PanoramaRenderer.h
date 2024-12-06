/**
* @file        :PanoramaRenderer.h
* @brief       :360 全景渲染器类
* @details     :支持全景图像，视频渲染，支持透视图，小行星，水晶球视角看全景，支持鼠标拖动旋转全景，支持滚轮缩放视野
* @date        :2024/12/06 15:08:30
* @author      :cuixingxing(cuixingxing150@gmail.com)
* @version     :1.0
*
* @copyright Copyright (c) 2024
*
*/

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

#define USE_GL_BEGIN_END 0

// 照片动画N个节点，N-1个区间段，首尾节点数据保持一致，表示回到原处状态
template <int N>
struct AnimationEffect {
    glm::vec3 CameraPosNodes[N];  // 动画在N个节点上的相机位置
    float PitchNodes[N];          // 动画在N个节点上的pitch角度
    float YawNodes[N];            // 动画在N个节点上的yaw角度
    float FovNodes[N];            // 动画在N个节点上的fov角度

    float stagesDuration[N - 1];  // 每个阶段的时长（N-1个阶段）
    bool isLooping;               // 是否循环播放

    // 计算动画的总时长
    float getTotalDuration() const {
        float totalDuration = 0.0f;
        for (int i = 0; i < N - 1; i++) {
            totalDuration += stagesDuration[i];
        }
        return totalDuration;
    }

    // 获取当前阶段的插值进度
    float getStageProgress(float currentTime) const {
        float accumulatedTime = 0.0f;

        for (int i = 0; i < N - 1; i++) {
            accumulatedTime += stagesDuration[i];
            if (currentTime <= accumulatedTime) {
                float stageStartTime = accumulatedTime - stagesDuration[i];
                return glm::clamp((currentTime - stageStartTime) / stagesDuration[i], 0.0f, 1.0f);
            }
        }
        return 1.0f;  // 动画结束
    }

    // 获取当前阶段的参数（例如：相机位置，pitch, yaw, fov）
    void getInterpolatedParams(float currentTime, glm::vec3 &cameraPos, float &pitch, float &yaw, float &fov) const {
        float progress = getStageProgress(currentTime);

        for (int i = 0; i < N - 1; i++) {
            float stageStartTime = i > 0 ? stagesDuration[i - 1] : 0.0f;
            if (currentTime <= stageStartTime + stagesDuration[i]) {
                // 线性插值计算相机位置和视角
                cameraPos = glm::mix(CameraPosNodes[i], CameraPosNodes[i + 1], progress);
                pitch = glm::mix(PitchNodes[i], PitchNodes[i + 1], progress);
                yaw = glm::mix(YawNodes[i], YawNodes[i + 1], progress);
                fov = glm::mix(FovNodes[i], FovNodes[i + 1], progress);
                break;
            }
        }
    }
};

class PanoramaRenderer {
   public:
    enum class SwitchMode { PANORAMAVIDEO,
                            PANORAMAIMAGE };  //全景视频，全景图像
    enum class ViewMode { PERSPECTIVE,
                          LITTLEPLANET,
                          CRYSTALBALL };  // 透视图,小行星，水晶球视角看全景

    enum class PanoAnimator { NONE,
                              ROTATE,
                              SWIPE,
                              SWIPE_ROTATE };  //全景动画类型,仅仅全景照片适用
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

    GLFWwindow *m_window;
    // 全景图片和视频渲染
    GLuint m_vao, m_vboVertices, m_vboIndices, m_vboTexCoords;  // 顶点数组对象和缓冲对象
    GLuint m_shaderProgram, m_texture;                          // 着色器程序和纹理对象

    glm::mat4 m_projection;
    glm::mat4 m_view;

    ViewMode m_viewOrientation;   // 透视图，小行星，水晶球
    PanoAnimator m_panoAnimator;  // 全景动画类型,仅仅全景照片适用
    SwitchMode m_panoMode;        // 全景视频，全景图像切换

    // 播放屏幕宽和高尺寸
    int m_widthScreen;
    int m_heightScreen;

    glm::vec3 m_cameraPosition;
    float m_pitch, m_yaw, m_prevPitch;  // 摄像机旋转角度
    float m_fov;                        // 初始视野角度
    bool m_isDragging;                  // 是否正在拖动鼠标
    double m_lastX, m_lastY;            // 上次鼠标的位置
    SphereData *m_sphereData;
    cv::VideoCapture m_videoCapture;

    // 照片动画师
    AnimationEffect<4> m_animationEffect;  // 三阶段的动画效果
    float m_animationTime = 0.0f;          // 当前动画的计时器
};

#endif  // PANORAMARENDERER_H
