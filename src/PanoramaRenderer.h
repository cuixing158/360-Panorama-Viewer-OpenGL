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

#include <iostream>
#include <thread>
#include <atomic>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "Sphere.h"

#define USE_GL_BEGIN_END 0

// 照片动画N个节点，N-1个区间段，如果首尾节点数据保持一致，表示回到原处状态
struct AnimationEffect {
    std::vector<glm::vec3> CameraPosNodes;  // 动画在N个节点上的相机位置
    std::vector<glm::quat> CameraRotNodes;  // 动画在N个节点上的相机朝向四元数
    std::vector<float> FovNodes;            // 动画在N个节点上的fov角度

    std::vector<float> stagesDuration;  // 每个阶段的时长（N-1个阶段）,长度比上面数组少1

    // 计算动画的总时长
    float getTotalDuration() const {
        float totalDuration = 0.0f;
        for (size_t i = 0; i < stagesDuration.size(); i++) {
            totalDuration += stagesDuration[i];
        }
        return totalDuration;
    }

    // 获取当前阶段的插值进度
    float getStageProgress(float currentTime) const {
        float accumulatedTime = 0.0f;

        for (size_t i = 0; i < stagesDuration.size(); i++) {
            accumulatedTime += stagesDuration[i];
            if (currentTime <= accumulatedTime) {
                float stageStartTime = accumulatedTime - stagesDuration[i];
                return glm::clamp((currentTime - stageStartTime) / stagesDuration[i], 0.0f, 1.0f);
            }
        }
        return 1.0f;  // 动画结束
    }

    // 获取当前阶段的参数（例如：相机位置，方向，fov）
    void getInterpolatedParams(float currentTime, glm::vec3 &cameraPos, glm::quat &cameraRot, float &fov) const {
        float progress = getStageProgress(currentTime);

        // 处理插值
        float accumulatedStageTime = 0.0f;
        for (size_t i = 0; i < stagesDuration.size(); i++) {
            float stageStartTime = accumulatedStageTime;
            accumulatedStageTime += stagesDuration[i];  // 累加前面的阶段时长

            if (currentTime <= stageStartTime + stagesDuration[i]) {
                // 线性插值计算相机位置和fov
                cameraPos = glm::mix(CameraPosNodes[i], CameraPosNodes[i + 1], progress);
                fov = glm::mix(FovNodes[i], FovNodes[i + 1], progress);

                // 使用slerp对四元数进行插值，计算相机朝向
                cameraRot = glm::slerp(CameraRotNodes[i], CameraRotNodes[i + 1], progress);
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

    // 导出“照片动画师”为视频
    void exportAnimationEffectThread(const std::string &outputFile, int width, int height, int fps);  // 导出动画视频函数声明
    void exportAnimationEffect(const std::string &outputFile, int width, int height, int fps);        // 导出动画视频函数声明
    void startExportAnimationEffect(const std::string &outputFile, int width, int height, int fps);   // 启动后台线程导出

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
    void getViewMatrixForStatic(glm::mat4 &projection, glm::mat4 &view);
    // 由当前的相机位置，方向，fov获取视图矩阵
    void getViewMatrixForAnimation(glm::vec3 cameraPos, glm::quat cameraRot, float fov, glm::mat4 &projection, glm::mat4 &view);
    void renderPanorama(SphereData *sphereData, glm::mat4 projection, glm::mat4 view);
    // 鼠标按下和移动回调函数
    void mouse_callback(double xpos, double ypos);
    // 鼠标按下回调函数
    void mouse_button_callback(int button, int action, int mods);
    // 滚轮回调函数（用于调整 FOV）
    void scroll_callback(double xoffset, double yoffset);

    GLFWwindow *m_window;  // 主线程中的窗口
    // 全景图片和视频渲染
    GLuint m_vao, m_vboVertices, m_vboIndices, m_vboTexCoords;  // 顶点数组对象和缓冲对象
    GLuint m_shaderProgram, m_texture;                          // 着色器程序和纹理对象

    ViewMode m_viewOrientation;   // 透视图，小行星，水晶球
    PanoAnimator m_panoAnimator;  // 全景动画类型,仅仅全景照片适用
    SwitchMode m_panoMode;        // 全景视频，全景图像切换

    // 播放屏幕宽和高尺寸
    int m_widthScreen;
    int m_heightScreen;

    float m_pitch, m_yaw, m_prevPitch;  // 摄像机旋转角度,适合手动交互时候使用的变量
    float m_fov;                        // 初始视野角度,适合手动交互时候使用的变量
    bool m_isDragging;                  // 是否正在拖动鼠标,适合手动交互时候使用的变量
    double m_lastX, m_lastY;            // 上次鼠标的位置,适合手动交互时候使用的变量
    SphereData *m_sphereData;
    cv::VideoCapture m_videoCapture;

    // 照片动画师
    AnimationEffect m_animationEffect;  // 三阶段的动画效果
    float m_animationTime = 0.0f;       // 当前动画的计时器
    float m_lastFrameTime;              // 上一帧的时间戳

    // 导出视频的后台线程
    std::atomic<bool> m_exporting;  // 用于检测是否正在导出
    std::thread m_exportThread;     // 后台导出线程
};

#endif  // PANORAMARENDERER_H
