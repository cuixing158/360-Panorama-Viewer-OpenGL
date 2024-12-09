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

// // 钩子函数，从ff_ffplay.c中执行
// cv::Mat PanoramaRenderer::frame;
// std::mutex PanoramaRenderer::textureMutex;
// panorama::DualFisheyeSticher initializeSticher();
// panorama::DualFisheyeSticher sticher = initializeSticher();

// panorama::DualFisheyeSticher initializeSticher() {
//     // 360 全景拼接初始化,下面参数适合insta360 设备的
//     panorama::cameraParam cam1, cam2;
//     cv::Size outputSize = cv::Size(2000, 1000);
//     float hemisphereWidth = 960.0f;                                                       //OBS推流是960.0f
//     cam1.circleFisheyeImage = cv::Mat::zeros(hemisphereWidth, hemisphereWidth, CV_8UC3);  // 前单个球
//     cam1.FOV = 189.2357;
//     cam1.centerPt = cv::Point2f(hemisphereWidth / 2.0, hemisphereWidth / 2.0);
//     cam1.radius = hemisphereWidth / 2.0;
//     cam1.rotateX = 0.01112242;
//     cam1.rotateY = 0.2971962;
//     cam1.rotateZ = -0.0007757799;

//     // cam2
//     cam2.circleFisheyeImage = cv::Mat::zeros(hemisphereWidth, hemisphereWidth, CV_8UC3);  // 后单个球;
//     cam2.FOV = 194.1712;
//     cam2.centerPt = cv::Point2f(hemisphereWidth / 2.0, hemisphereWidth / 2.0);
//     cam2.radius = hemisphereWidth / 2.0;
//     cam2.rotateX = -0.7172632;
//     cam2.rotateY = 0.5694329;
//     cam2.rotateZ = 179.9732;
//     panorama::DualFisheyeSticher sticher = panorama::DualFisheyeSticher(cam1, cam2, outputSize);
//     return sticher;
// }

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

    if (m_panoMode == SwitchMode::PANORAMAIMAGE)  // 照片动画师功能
    {
        if (glfwGetKey(m_window, GLFW_KEY_F1) == GLFW_PRESS) {
            // 启动第一种动画效果

            m_animationTime = 0.0f;  // 重置动画时间
            m_panoAnimator = PanoramaRenderer::PanoAnimator::ROTATE;
            // 创建一个4个节点3个阶段动画效果
            m_animationEffect =  // 创建一个4节点的动画效果（3个阶段）
                {
                    // 节点的相机位置
                    {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)},
                    // 节点的pitch, m_yaw, m_fov
                    {0.0f, 0.0f, 90.0f, 0.0f},
                    {0.0f, 360.0f, 0.0f, 0.0f},
                    {60.0f, 60.0f, 120.0f, 60.0f},

                    // 每个阶段的时长
                    {10.0f, 2.0f, 3.0f},

                    false  // 不循环
                };

        } else if (glfwGetKey(m_window, GLFW_KEY_F2) == GLFW_PRESS) {
            // 启动第二种动画效果
            m_animationTime = 0.0f;  // 重置动画时间
        } else if (glfwGetKey(m_window, GLFW_KEY_F3) == GLFW_PRESS) {
            // 启动第三种动画效果
            m_animationTime = 0.0f;  // 重置动画时间
        }
    }

    // 只有透视图才限制俯仰角度
    if (m_viewOrientation == PanoramaRenderer::ViewMode::PERSPECTIVE) {
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

// 设置视图矩阵
void PanoramaRenderer::getViewMatrix(glm::mat4 &m_projection, glm::mat4 &m_view) {
    static glm::vec3 upCamera = glm::vec3(0.0f, 1.0f, 0.0f);
    // 设置投影矩阵
    m_projection = glm::perspective(glm::radians(m_fov), (float)m_widthScreen / m_heightScreen, 0.1f, 100.0f);

    // 根据当前视角模式设置视图矩阵
    glm::vec3 movingPosition(sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)), sin(glm::radians(m_pitch)), cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)));  // 移动视角位置
    if (m_viewOrientation == PanoramaRenderer::ViewMode::PERSPECTIVE) {
        m_cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        m_view = glm::lookAt(m_cameraPosition, movingPosition,
                             glm::vec3(0, 1, 0));
    } else if (m_viewOrientation == PanoramaRenderer::ViewMode::LITTLEPLANET) {
        m_cameraPosition = movingPosition;  // 在单位球表面

        if (hasDivisibleNode(m_prevPitch, m_pitch)) {
            upCamera[1] = upCamera[1] * -1.0f;
        }

        m_view = glm::lookAt(m_cameraPosition, glm::vec3(0.0f, 0.0f, 0.0f), upCamera);
        m_prevPitch = m_pitch;
    } else if (m_viewOrientation == PanoramaRenderer::ViewMode::CRYSTALBALL) {
        m_cameraPosition = 1.5f * movingPosition;  // 球外部
        if (hasDivisibleNode(m_prevPitch, m_pitch)) {
            upCamera[1] = upCamera[1] * -1.0f;
        }
        m_view = glm::lookAt(m_cameraPosition, glm::vec3(0.0f, 0.0f, 0.0f), upCamera);

        m_prevPitch = m_pitch;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(m_projection));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(m_view));
}

void PanoramaRenderer::renderPanorama(SphereData *m_sphereData, glm::mat4 m_projection, glm::mat4 m_view) {
    glUseProgram(m_shaderProgram);

    // 设置 uniform 矩阵
    GLuint projLoc = glGetUniformLocation(m_shaderProgram, "m_projection");
    GLuint viewLoc = glGetUniformLocation(m_shaderProgram, "m_view");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(m_projection));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(m_view));

    // 绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glUniform1i(glGetUniformLocation(m_shaderProgram, "texture1"), 0);

    // 绘制球体
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_sphereData->getNumIndices(), GL_UNSIGNED_SHORT, 0);
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

        // 待修复step2中的， m_cameraPosition, m_pitch, m_yaw, m_fov引起后续的m_view矩阵的计算正确性
        //                                                         // // step2 获取动画进度和当前相机参数
        //                                                         // if ((m_panoMode == SwitchMode::PANORAMAIMAGE) && (m_panoAnimator != PanoramaRenderer::PanoAnimator::NONE)) {
        //                                                         //     float currentFrame = glfwGetTime();                // 获取当前时间
        //                                                         //     float deltaTime = currentFrame - m_lastFrameTime;  // 计算帧间时间
        //                                                         //     m_lastFrameTime = currentFrame;
        //                                                         //     // 更新动画时间
        //                                                         //     m_animationTime += deltaTime;

        //                                                         //     // 获得当前动画节点的相机参数，m_cameraPosition, m_pitch, m_yaw, m_fov
        //                                                         //     m_animationEffect.getInterpolatedParams(m_animationTime, m_cameraPosition, m_pitch, m_yaw, m_fov);
        //                                                         // }

        // step3 设置视图矩阵
        glm::mat4 m_projection,
            m_view;
        getViewMatrix(m_projection, m_view);  // 获取投影和视角矩阵

// step4 渲染
#if USE_GL_BEGIN_END
        renderSphere(1.0f, 50, 50);
#else
        renderPanorama(m_sphereData, m_projection, m_view);
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
    : m_window(nullptr), m_vao(0), m_vboVertices(0), m_vboIndices(0), m_vboTexCoords(0), m_shaderProgram(0), m_texture(0), m_projection(glm::mat4(1.0)), m_view(glm::mat4(1.0)), m_viewOrientation(ViewMode::PERSPECTIVE), m_panoAnimator(PanoAnimator::NONE), m_panoMode(SwitchMode::PANORAMAIMAGE), m_widthScreen(1920), m_heightScreen(1080), m_pitch(0.0f), m_yaw(0.0f), m_prevPitch(0.0f), m_fov(60.0f), m_isDragging(false), m_lastX(0), m_lastY(0), m_sphereData(new SphereData(1.0f, 50, 50)) {
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
