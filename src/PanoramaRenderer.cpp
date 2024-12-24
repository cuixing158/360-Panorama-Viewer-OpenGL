/**
* @file        :PanoramaRenderer.cpp
* @brief       :360 全景渲染器类实现
* @details     :支持全景图像，视频渲染，支持透视图，小行星，水晶球视角看全景，支持鼠标拖动旋转全景，支持滚轮缩放视野
* @date        :2024/12/06 15:08:30
* @author      :cuixingxing(cuixingxing150@gmail.com)
* @version     :1.0
*
* @copyright Copyright (c) 2024
*
*/
#include "PanoramaRenderer.h"

// Function to create a shader program
GLuint PanoramaRenderer::createProgram(const char *vertexSource, const char *fragmentSource) {
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

void PanoramaRenderer::initPanoramaRenderer() {
    const char *vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    uniform mat4 m_projection;
    uniform mat4 m_view;
    void main() {
        TexCoord = aTexCoord;
        gl_Position = m_projection * m_view * vec4(aPos, 1.0);
    }
)";

    const char *fragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D texture1;
    void main() {
        FragColor = texture(texture1, TexCoord);
    }
)";

    // 创建着色器程序
    m_shaderProgram = createProgram(vertexShaderSource, fragmentShaderSource);

    // 生成 VAO 和 VBO
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vboVertices);
    glGenBuffers(1, &m_vboIndices);
    glGenBuffers(1, &m_vboTexCoords);

    // 绑定 VAO
    glBindVertexArray(m_vao);

    // 顶点数据
    glBindBuffer(GL_ARRAY_BUFFER, m_vboVertices);
    glBufferData(GL_ARRAY_BUFFER, m_sphereData->getNumVertices() * sizeof(GLfloat), m_sphereData->getVertices(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    // 纹理坐标数据
    glBindBuffer(GL_ARRAY_BUFFER, m_vboTexCoords);
    glBufferData(GL_ARRAY_BUFFER, m_sphereData->getNumTexs() * sizeof(GLfloat), m_sphereData->getTexCoords(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    // 索引数据
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_sphereData->getNumIndices() * sizeof(GLushort), m_sphereData->getIndices(), GL_STATIC_DRAW);

    // 解绑 VAO
    glBindVertexArray(0);
}

// 绘制球体，该函数是传统的立即模式渲染函数glBegin/glEnd，现代OpenGL中不推荐使用
void PanoramaRenderer::renderSphere(float radius, int slices, int stacks) {
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
void PanoramaRenderer::processInput() {
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) m_pitch += 0.5f;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) m_pitch -= 0.5f;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) m_yaw -= 0.5f;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) m_yaw += 0.5f;

    if (glfwGetKey(m_window, GLFW_KEY_1) == GLFW_PRESS) {
        m_viewOrientation = PanoramaRenderer::ViewMode::PERSPECTIVE;
        m_panoAnimator = PanoramaRenderer::PanoAnimator::NONE;
        m_pitch = 0.0f;
        m_prevPitch = 0.0f;
        m_yaw = 0.0f;
        m_fov = 60.0f;
    }

    if (glfwGetKey(m_window, GLFW_KEY_2) == GLFW_PRESS) {
        m_viewOrientation = PanoramaRenderer::ViewMode::LITTLEPLANET;
        m_panoAnimator = PanoramaRenderer::PanoAnimator::NONE;
        m_pitch = 90.0f;
        m_prevPitch = m_pitch;
        m_yaw = 0.0f;
        m_fov = 120.0f;
    }
    if (glfwGetKey(m_window, GLFW_KEY_3) == GLFW_PRESS) {
        m_viewOrientation = PanoramaRenderer::ViewMode::CRYSTALBALL;
        m_panoAnimator = PanoramaRenderer::PanoAnimator::NONE;
        m_pitch = 0.0f;
        m_prevPitch = m_pitch;
        m_yaw = 0.0f;
        m_fov = 85.0f;
    }

    // 加入键盘快捷键，保存导出的全景照片动画师效果,但不影响主线程运行
    if (glfwGetKey(m_window, GLFW_KEY_P) == GLFW_PRESS) {
        double t1 = cv::getTickCount();
        exportAnimationEffect("panoAnimator.mp4", 1920, 1080, 30);
        printf("it take time:%f seconds.\n", (cv::getTickCount() - t1) / cv::getTickFrequency());
        // startExportAnimationEffect("panoAnimator.mp4", 1920, 1080, 30);
    }

    // 处理全景照片动画师功能
    if (m_panoMode == SwitchMode::PANORAMAIMAGE)  // 照片动画师功能
    {
        if (glfwGetKey(m_window, GLFW_KEY_F1) == GLFW_PRESS) {
            // 启动第一种动画效果，360度四周变化
            m_animationTime = 0.0f;  // 重置动画时间

            m_panoAnimator = PanoramaRenderer::PanoAnimator::ROTATE;

            // 创建一个6节点、5个阶段的动画效果
            glm::vec3 eulerAngles0(0.0f, glm::radians(0.0f), 0.0f);  // 0度绕X, 0度绕Y, 0度绕Z
            glm::quat rotationQuaternion0(eulerAngles0);             // 创建旋转四元数

            glm::vec3 eulerAngles1(0.0f, glm::radians(180.0f), 0.0f);  // 旋转180度绕Y轴
            glm::quat rotationQuaternion1(eulerAngles1);               // 创建旋转四元数

            glm::vec3 eulerAngles2(0.0f, glm::radians(360.0f), 0.0f);  // 旋转360度绕Y轴
            glm::quat rotationQuaternion2(eulerAngles2);               // 创建旋转四元数

            glm::vec3 eulerAngles3(-glm::radians(45.0f), glm::radians(180.0f), 0.0f);
            glm::quat rotationQuaternion3(eulerAngles3);  // 创建旋转四元数

            glm::vec3 eulerAngles4(-glm::radians(90.0f), glm::radians(360.0f), 0.0f);
            glm::quat rotationQuaternion4(eulerAngles4);  // 创建旋转四元数

            glm::vec3 eulerAngles5(0.0f, glm::radians(0.0f), 0.0f);  // 回到起始点
            glm::quat rotationQuaternion5(eulerAngles5);             // 创建旋转四元数

            m_animationEffect.CameraPosNodes = {
                // 节点的相机位置
                glm::vec3(0.0f, 0.0f, 0.0f),  // 第1个节点
                glm::vec3(0.0f, 0.0f, 0.0f),  // 第2个节点
                glm::vec3(0.0f, 0.0f, 0.0f),  // 第3个节点
                glm::vec3(0.0f, 0.5f, 0.0f),  // 第4个节点
                glm::vec3(0.0f, 1.0f, 0.0f),  // 第5个节点
                glm::vec3(0.0f, 0.0f, 0.0f)   // 第6个节点
            };

            m_animationEffect.CameraRotNodes = {
                // 节点的相机朝向四元数
                rotationQuaternion0,  // 第1个节点的旋转
                rotationQuaternion1,  // 第2个节点的旋转
                rotationQuaternion2,  // 第3个节点的旋转
                rotationQuaternion3,  // 第4个节点的旋转
                rotationQuaternion4,  // 第5个节点的旋转
                rotationQuaternion5   // 第6个节点的旋转
            };

            m_animationEffect.FovNodes = {                                             // 节点的FOV
                                          60.0f, 60.0f, 60.0f, 90.0f, 120.0f, 60.0f};  // FOV值为60, 60, 120, 60度

            m_animationEffect.stagesDuration = {                                // 每个阶段的时长
                                                4.0f, 4.0f, 1.0f, 1.0f, 1.0f};  // 阶段1：10秒，阶段2：2秒，阶段3：3秒
        } else if (glfwGetKey(m_window, GLFW_KEY_F2) == GLFW_PRESS) {
            // 启动第二种动画效果，地变天视图
            m_animationTime = 0.0f;  // 重置动画时间

            m_panoAnimator = PanoramaRenderer::PanoAnimator::SWIPE;

            // 创建一个4节点、3个阶段的动画效果
            glm::vec3 eulerAngles0(-glm::radians(90.0f), glm::radians(0.0f), 0.0f);  // 0度绕X, 0度绕Y, 0度绕Z
            glm::quat rotationQuaternion0(eulerAngles0);                             // 创建旋转四元数

            glm::vec3 eulerAngles1(0.0f, glm::radians(180.0f), 0.0f);  // 旋转90度绕Y轴
            glm::quat rotationQuaternion1(eulerAngles1);               // 创建旋转四元数

            glm::vec3 eulerAngles2(glm::radians(90.0f), glm::radians(360.0f), 0.0f);  // 旋转360度绕Y轴
            glm::quat rotationQuaternion2(eulerAngles2);                              // 创建旋转四元数

            glm::vec3 eulerAngles3(0.0f, glm::radians(0.0f), 0.0f);  // 旋转270度绕Y轴
            glm::quat rotationQuaternion3(eulerAngles3);             // 创建旋转四元数

            m_animationEffect.CameraPosNodes = {
                // 节点的相机位置
                glm::vec3(0.0f, 1.0f, 0.0f),   // 第1个节点
                glm::vec3(0.0f, 0.0f, 0.0f),   // 第2个节点
                glm::vec3(0.0f, -1.0f, 0.0f),  // 第3个节点
                glm::vec3(0.0f, 0.0f, 0.0f)    // 第4个节点
            };

            m_animationEffect.CameraRotNodes = {
                // 节点的相机朝向四元数

                rotationQuaternion0,  // 第1个节点的旋转
                rotationQuaternion1,  // 第2个节点的旋转
                rotationQuaternion2,  // 第3个节点的旋转
                rotationQuaternion3   // 第4个节点的旋转
            };

            m_animationEffect.FovNodes = {                                // 节点的FOV
                                          120.0f, 60.0f, 120.0f, 80.0f};  // FOV值为60, 60, 120, 60度

            m_animationEffect.stagesDuration = {                    // 每个阶段的时长
                                                5.0f, 2.0f, 2.0f};  // 阶段1：10秒，阶段2：2秒，阶段3：3秒

        } else if (glfwGetKey(m_window, GLFW_KEY_F3) == GLFW_PRESS) {
            // 启动第三种动画效果,天变地视图
            m_animationTime = 0.0f;  // 重置动画时间

            m_panoAnimator = PanoramaRenderer::PanoAnimator::SWIPE_ROTATE;

            // 创建一个4节点、3个阶段的动画效果
            glm::vec3 eulerAngles0(glm::radians(90.0f), glm::radians(0.0f), 0.0f);  // 0度绕X, 90度绕Y, 0度绕Z
            glm::quat rotationQuaternion0(eulerAngles0);                            // 创建旋转四元数

            glm::vec3 eulerAngles1(glm::radians(90.0f), glm::radians(0.0f), 0.0f);  //
            glm::quat rotationQuaternion1(eulerAngles1);                            // 创建旋转四元数

            glm::vec3 eulerAngles2(0.0f, glm::radians(180.0f), 0.0f);  // 旋转90度绕Y轴
            glm::quat rotationQuaternion2(eulerAngles2);               // 创建旋转四元数

            glm::vec3 eulerAngles3(-glm::radians(90.0f), glm::radians(360.0f), 0.0f);  //
            glm::quat rotationQuaternion3(eulerAngles3);                               // 创建旋转四元数

            glm::vec3 eulerAngles4(0.0f, glm::radians(0.0f), 0.0f);  //
            glm::quat rotationQuaternion4(eulerAngles4);             // 创建旋转四元数

            m_animationEffect.CameraPosNodes = {
                // 节点的相机位置
                glm::vec3(0.0f, -1.0f, 0.0f),  // 第1个节点
                glm::vec3(0.0f, -1.0f, 0.0f),  // 第2个节点
                glm::vec3(0.0f, 0.0f, 0.0f),   // 第3个节点
                glm::vec3(0.0f, 1.0f, 0.0f),   // 第4个节点
                glm::vec3(0.0f, 0.0f, 0.0f)    // 第5个节点
            };

            m_animationEffect.CameraRotNodes = {
                // 节点的相机朝向四元数

                rotationQuaternion0,  // 第1个节点的旋转
                rotationQuaternion1,  // 第2个节点的旋转
                rotationQuaternion2,  // 第3个节点的旋转
                rotationQuaternion3,  // 第4个节点的旋转
                rotationQuaternion4   // 第5个节点的旋转
            };

            m_animationEffect.FovNodes = {                                        // 节点的FOV
                                          120.0f, 110.0f, 60.0f, 120.0f, 60.0f};  // FOV值为120, 110, 60, 60, 120, 60度

            m_animationEffect.stagesDuration = {                          // 每个阶段的时长
                                                1.5f, 3.0f, 2.0f, 2.0f};  // 阶段1：10秒，阶段2：2秒，阶段3：3秒
        }
    }

    // 只有在手动交互式的透视图才限制俯仰角度
    if ((m_viewOrientation == PanoramaRenderer::ViewMode::PERSPECTIVE) && (m_panoAnimator == PanoramaRenderer::PanoAnimator::NONE)) {
        m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);
    }
    m_yaw = glm::mod(m_yaw, 360.0f);
}

bool PanoramaRenderer::hasDivisibleNode(float previousPitch, float m_pitch) {
    // 确保 previousPitch 小于 m_pitch
    if (previousPitch > m_pitch) std::swap(previousPitch, m_pitch);

    // 不包括边界
    float lowerBound = previousPitch + std::numeric_limits<float>::epsilon();
    float upperBound = m_pitch - std::numeric_limits<float>::epsilon();

    // 找到第一个大于 lowerBound 的 "90 + 180*k" 的数
    float start = 90.0f + std::ceil((lowerBound - 90.0f) / 180.0f) * 180.0f;

    // 判断 start 是否在范围内
    return start > lowerBound && start < upperBound;
}

// 根据手动交互得到的m_pitch,m_yaw得到视图矩阵
void PanoramaRenderer::getViewMatrixForStatic(glm::mat4 &projection, glm::mat4 &view) {
    static glm::vec3 upCamera = glm::vec3(0.0f, 1.0f, 0.0f);
    // 设置投影矩阵
    projection = glm::perspective(glm::radians(m_fov), (float)m_widthScreen / m_heightScreen, 0.1f, 100.0f);

    // 根据当前视角模式设置视图矩阵
    glm::vec3 movingPosition(sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)), sin(glm::radians(m_pitch)), cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)));  // 移动视角位置
    glm::vec3 cameraPosition;
    if (m_viewOrientation == PanoramaRenderer::ViewMode::PERSPECTIVE) {
        cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        view = glm::lookAt(cameraPosition, movingPosition,
                           glm::vec3(0, 1, 0));
    } else if (m_viewOrientation == PanoramaRenderer::ViewMode::LITTLEPLANET) {
        cameraPosition = movingPosition;  // 在单位球表面

        if (hasDivisibleNode(m_prevPitch, m_pitch)) {
            upCamera[1] = upCamera[1] * -1.0f;
        }

        view = glm::lookAt(cameraPosition, glm::vec3(0.0f, 0.0f, 0.0f), upCamera);
        m_prevPitch = m_pitch;
    } else if (m_viewOrientation == PanoramaRenderer::ViewMode::CRYSTALBALL) {
        cameraPosition = 1.5f * movingPosition;  // 球外部
        if (hasDivisibleNode(m_prevPitch, m_pitch)) {
            upCamera[1] = upCamera[1] * -1.0f;
        }
        view = glm::lookAt(cameraPosition, glm::vec3(0.0f, 0.0f, 0.0f), upCamera);

        m_prevPitch = m_pitch;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(projection));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(view));
}

// 获取动态视图矩阵,照片动画师功能
void PanoramaRenderer::getViewMatrixForAnimation(glm::vec3 cameraPos, glm::quat cameraRot, float fov, glm::mat4 &projection, glm::mat4 &view) {
    // 计算投影矩阵
    projection = glm::perspective(glm::radians(fov), (float)m_widthScreen / m_heightScreen, 0.1f, 100.0f);

    // 计算视图矩阵
    // 获取相机的前向、右向和上向向量
    glm::vec3 forward = glm::normalize(cameraRot * glm::vec3(0.0f, 0.0f, -1.0f));  // 朝向
    // glm::vec3 right = glm::normalize(cameraRot * glm::vec3(1.0f, 0.0f, 0.0f));     // 右向
    glm::vec3 up = glm::normalize(cameraRot * glm::vec3(0.0f, 1.0f, 0.0f));  // 上向

    // 视图矩阵：使用 lookAt 来创建视图矩阵
    glm::vec3 target = cameraPos + forward;     // 目标点是当前相机位置加上朝向向量
    view = glm::lookAt(cameraPos, target, up);  // 计算视图矩阵

    // 将视图矩阵应用于OpenGL
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(projection));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(view));
}

void PanoramaRenderer::renderPanorama(SphereData *sphereData, glm::mat4 projection, glm::mat4 view) {
    glUseProgram(m_shaderProgram);

    // 设置 uniform 矩阵
    GLuint projLoc = glGetUniformLocation(m_shaderProgram, "m_projection");
    GLuint viewLoc = glGetUniformLocation(m_shaderProgram, "m_view");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // 绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "texture1"), 0);

    // 绘制球体
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, sphereData->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);

    glUseProgram(0);
}

// 渲染循环
void PanoramaRenderer::renderLoop() {
    while (!glfwWindowShouldClose(m_window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // step1, 处理用户输入
        processInput();
        if (m_panoMode == SwitchMode::PANORAMAVIDEO) {
            updateVideoFrame();
        }

        // 计算projection和view矩阵
        // step2 获取动画进度和当前相机参数 // step3 设置视图矩阵
        glm::mat4 projection, view;
        if ((m_panoMode == SwitchMode::PANORAMAIMAGE) && (m_panoAnimator != PanoramaRenderer::PanoAnimator::NONE)) {
            float currentFrameTime = cv::getTickCount();                                      // 获取当前时间
            float deltaTime = (currentFrameTime - m_lastFrameTime) / cv::getTickFrequency();  // 计算帧间时间
            m_lastFrameTime = currentFrameTime;
            // 更新动画时间
            m_animationTime += deltaTime;

            // 获得当前动画节点的相机参数，m_cameraPosition,, m_fov
            glm::vec3 cameraPosition;
            glm::quat cameraOrientation;
            float fov;
            m_animationEffect.getInterpolatedParams(m_animationTime, cameraPosition, cameraOrientation, fov);

            getViewMatrixForAnimation(cameraPosition, cameraOrientation, fov, projection, view);  // 获取投影和视角矩阵, 动画视角
        } else {
            getViewMatrixForStatic(projection, view);  // 获取投影和视角矩阵, 静态视角
        }

// step4 渲染
#if USE_GL_BEGIN_END
        renderSphere(1.0f, 50, 50);
#else
        renderPanorama(m_sphereData, projection, view);
#endif

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }
}

void PanoramaRenderer::mouse_callback(double xpos, double ypos) {
    if (m_isDragging) {
        float xoffset = xpos - m_lastX;
        float yoffset = m_lastY - ypos;  // Y轴是反向的
        m_lastX = xpos;
        m_lastY = ypos;

        // 更新相机角度（偏航yaw和俯仰pitch）
        float sensitivity = 0.2f;  // 鼠标灵敏度
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        m_yaw += xoffset;
        m_pitch += yoffset;
    }
}

void PanoramaRenderer::mouse_button_callback(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            m_isDragging = true;
            glfwGetCursorPos(m_window, &m_lastX, &m_lastY);  // 记录鼠标按下时的位置
        }
        if (action == GLFW_RELEASE) {
            m_isDragging = false;  // 释放鼠标时停止拖动
        }
    }
}

void PanoramaRenderer::scroll_callback(double xoffset, double yoffset) {
    m_fov -= 4.0f * static_cast<float>(yoffset);  // 鼠标滚轮垂直移动
    m_fov = glm::clamp(m_fov, 1.0f, 120.0f);      // 限制 FOV 的范围
}

bool PanoramaRenderer::isImageFile(const std::string &filepath) {
    std::string extensions[] = {".jpg", ".jpeg", ".png", ".bmp", ".tga"};
    for (const auto &ext : extensions) {
        if (filepath.size() >= ext.size() && filepath.compare(filepath.size() - ext.size(), ext.size(), ext) == 0) {
            return true;
        }
    }
    return false;
}

bool PanoramaRenderer::isVideoFile(const std::string &filepath) {
    std::string extensions[] = {".mp4", ".avi", ".mov", ".mkv"};
    for (const auto &ext : extensions) {
        if (filepath.size() >= ext.size() && filepath.compare(filepath.size() - ext.size(), ext.size(), ext) == 0) {
            return true;
        }
    }
    return false;
}

// 加载全景图像
GLuint PanoramaRenderer::loadTexture(const char *path) {
    cv::Mat image = cv::imread(path, cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "can not load image: " << path << std::endl;
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
void PanoramaRenderer::updateVideoFrame() {
    if (m_panoMode != SwitchMode::PANORAMAVIDEO || !m_videoCapture.isOpened()) return;

    cv::Mat frame;
    if (!m_videoCapture.read(frame)) {
        // 视频读取结束，循环播放
        m_videoCapture.set(cv::CAP_PROP_POS_FRAMES, 0);
        m_videoCapture.read(frame);
    }

    // 转换为 OpenGL 纹理格式
    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
    cv::flip(frame, frame, 0);

    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame.cols, frame.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, frame.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
PanoramaRenderer::PanoramaRenderer(std::string filepath)
    : m_window(nullptr), m_vao(0), m_vboVertices(0), m_vboIndices(0), m_vboTexCoords(0), m_shaderProgram(0), m_texture(0), m_viewOrientation(ViewMode::PERSPECTIVE), m_panoAnimator(PanoAnimator::NONE), m_panoMode(SwitchMode::PANORAMAIMAGE), m_widthScreen(1920), m_heightScreen(1080), m_pitch(0.0f), m_yaw(0.0f), m_prevPitch(0.0f), m_fov(60.0f), m_isDragging(false), m_lastX(0), m_lastY(0), m_sphereData(new SphereData(1.0f, 50, 50)), m_lastFrameTime((float)cv::getTickCount()), m_exporting(false) {
    if (!glfwInit()) {
        std::cerr << "GLFW init failed!" << std::endl;
        exit(-1);
    }

    m_window = glfwCreateWindow(m_widthScreen, m_heightScreen, "360 Panorama Viewer", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "create window failed!" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(m_window);
    glewInit();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    // 初始化 SphereData
    m_sphereData = new SphereData(1.0f, 50, 50);

    initPanoramaRenderer();

    // 检测文件类型
    if (isImageFile(filepath)) {
        // 处理全景图片
        m_panoMode = SwitchMode::PANORAMAIMAGE;
        m_texture = loadTexture(filepath.c_str());
    } else if (isVideoFile(filepath)) {
        // 处理全景视频
        m_panoMode = SwitchMode::PANORAMAVIDEO;
        m_videoCapture.open(filepath);
        if (!m_videoCapture.isOpened()) {
            std::cerr << "Cannot open video file: " << filepath << std::endl;
            exit(1);
        }
        // 提取第一帧作为初始纹理
        updateVideoFrame();
    } else {
        std::cerr << "Unknow file type: " << filepath << std::endl;
        exit(1);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);  // 解绑 VBO,360全景图像最好需要
    glBindVertexArray(0);              // 解绑VAO,360全景图像最好需要
    if (m_panoMode == SwitchMode::PANORAMAIMAGE) {
        glGenerateMipmap(GL_TEXTURE_2D);  // 全景图像需要 mipmap,但是视频渲染不使用 glGenerateMipmap,较少性能开销
    }

    // 启用深度测试，防止遮挡影响
    glEnable(GL_DEPTH_TEST);
    // 设置深度测试函数
    glDepthFunc(GL_LESS);

    // 设置回调函数,设置当前实例为窗口的用户指针
    glfwSetWindowUserPointer(m_window, this);

    // 注册回调函数
    glfwSetCursorPosCallback(m_window, [](GLFWwindow *m_window, double xpos, double ypos) {
        auto *renderer = static_cast<PanoramaRenderer *>(glfwGetWindowUserPointer(m_window));
        renderer->mouse_callback(xpos, ypos);
    });

    glfwSetMouseButtonCallback(m_window, [](GLFWwindow *m_window, int button, int action, int mods) {
        auto *renderer = static_cast<PanoramaRenderer *>(glfwGetWindowUserPointer(m_window));
        renderer->mouse_button_callback(button, action, mods);
    });

    glfwSetScrollCallback(m_window, [](GLFWwindow *m_window, double xoffset, double yoffset) {
        auto *renderer = static_cast<PanoramaRenderer *>(glfwGetWindowUserPointer(m_window));
        renderer->scroll_callback(xoffset, yoffset);
    });
}

// 启动后台导出线程
void PanoramaRenderer::startExportAnimationEffect(const std::string &outputFile, int width, int height, int fps) {
    if (m_exporting.load()) {
        std::cerr << "Export already in progress!" << std::endl;
        return;
    }
    m_exporting.store(true);  // 设置导出标志

    // 启动后台线程进行导出
    m_exportThread = std::thread(&PanoramaRenderer::exportAnimationEffect, this, outputFile, width, height, fps);
    m_exportThread.detach();  // 分离线程，避免阻塞主线程
}

// 后台导出视频的实现
void PanoramaRenderer::exportAnimationEffectThread(const std::string &outputFile, int width, int height, int fps) {
    // 确保在此线程中使用主线程的 OpenGL 上下文
    glfwMakeContextCurrent(m_window);  // 确保当前线程使用主上下文

    // 创建一个帧缓冲对象 (FBO)
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // 创建一个纹理来存储渲染结果
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_widthScreen, m_heightScreen, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // 创建并附加一个深度缓冲区
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_widthScreen, m_heightScreen);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer not complete! Error code: " << framebufferStatus << std::endl;
        switch (framebufferStatus) {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                std::cerr << "Incomplete attachment!" << std::endl;
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                std::cerr << "Missing attachment!" << std::endl;
                break;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                std::cerr << "Framebuffer unsupported!" << std::endl;
                break;
            default:
                std::cerr << "Unknown error!" << std::endl;  // 待解决
                break;
        }
        return;
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer not complete!" << std::endl;
        return;
    }

    // 创建并打开视频编码器
    cv::VideoWriter videoWriter(outputFile, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(width, height));
    if (!videoWriter.isOpened()) {
        std::cerr << "Cannot open video file for writing: " << outputFile << std::endl;
        return;
    }

    // 获取当前动画模式的结构体，根据时刻0到总时间T，快速生成渲染帧，然后写入视频文件
    float totalTime = m_animationEffect.getTotalDuration();
    for (float t = 0.0f; t < totalTime; t += 1.0f / fps) {
        glm::vec3 cameraPosition;
        glm::quat cameraOrientation;
        float fov;
        m_animationEffect.getInterpolatedParams(t, cameraPosition, cameraOrientation, fov);

        // 获取视图矩阵
        glm::mat4 projection, view;
        getViewMatrixForAnimation(cameraPosition, cameraOrientation, fov, projection, view);

        // 渲染到帧缓冲对象
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, m_widthScreen, m_heightScreen);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderPanorama(m_sphereData, projection, view);

        // 读取渲染结果
        cv::Mat renderFrame(m_heightScreen, m_widthScreen, CV_8UC3);
        glReadPixels(0, 0, m_widthScreen, m_heightScreen, GL_RGB, GL_UNSIGNED_BYTE, renderFrame.data);

        // OpenGL 坐标系和 OpenCV 坐标系不同，需要翻转
        cv::flip(renderFrame, renderFrame, 0);

        // 将 BGR 顺序转换为 RGB 顺序
        cv::cvtColor(renderFrame, renderFrame, cv::COLOR_BGR2RGB);

        // 调整大小到指定的输出参数宽和高
        cv::Mat frame;
        cv::resize(renderFrame, frame, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);

        // 写入视频文件
        videoWriter.write(frame);
    }

    // 解绑帧缓冲对象
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 删除帧缓冲对象和纹理
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);
    glDeleteRenderbuffers(1, &rbo);

    // 销毁 OpenGL 上下文
    glfwMakeContextCurrent(nullptr);

    // 导出结束，重置标志
    m_exporting.store(false);
}

void PanoramaRenderer::exportAnimationEffect(const std::string &outputFile, int width, int height, int fps) {
    if (m_panoMode != SwitchMode::PANORAMAIMAGE || m_panoAnimator == PanoramaRenderer::PanoAnimator::NONE) {
        std::cerr << "No animation effect to export!" << std::endl;
        return;
    }

    // 创建一个视频编码器
    cv::VideoWriter videoWriter(outputFile, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(width, height));
    if (!videoWriter.isOpened()) {
        std::cerr << "Cannot open video file for writing: " << outputFile << std::endl;
        return;
    }

    // 获取当前动画模式的结构体，根据时刻0到总时间T，快速生成渲染帧，然后写入视频文件
    float totalTime = m_animationEffect.getTotalDuration();
    for (float t = 0.0f; t < totalTime; t += 1.0f / fps) {
        glm::vec3 cameraPosition;
        glm::quat cameraOrientation;
        float fov;
        m_animationEffect.getInterpolatedParams(t, cameraPosition, cameraOrientation, fov);

        // 获取视图矩阵
        glm::mat4 projection, view;
        getViewMatrixForAnimation(cameraPosition, cameraOrientation, fov, projection, view);

        // 渲染
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderPanorama(m_sphereData, projection, view);

        // 读取渲染结果
        cv::Mat renderFrame(m_heightScreen, m_widthScreen, CV_8UC3);
        glReadPixels(0, 0, m_widthScreen, m_heightScreen, GL_RGB, GL_UNSIGNED_BYTE, renderFrame.data);

        // OpenGL 坐标系和 OpenCV 坐标系不同，需要翻转
        cv::flip(renderFrame, renderFrame, 0);

        // 将 BGR 顺序转换为 RGB 顺序
        cv::cvtColor(renderFrame, renderFrame, cv::COLOR_BGR2RGB);

        // 调整大小到指定的输出参数宽和高
        cv::Mat frame;
        cv::resize(renderFrame, frame, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);

        // 写入视频文件
        videoWriter.write(frame);
    }
}

PanoramaRenderer::~PanoramaRenderer() {
    delete m_sphereData;
    glDeleteProgram(m_shaderProgram);
    glDeleteTextures(1, &m_texture);
    // glDeleteTextures(1, &videoTexture);
    glDeleteBuffers(1, &m_vboVertices);
    glDeleteBuffers(1, &m_vboTexCoords);
    glDeleteBuffers(1, &m_vboIndices);
    glDeleteVertexArrays(1, &m_vao);

    glfwDestroyWindow(m_window);
    glfwTerminate();
}
