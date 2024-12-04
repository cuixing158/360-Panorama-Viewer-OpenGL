#include "PanoramaRenderer.h"

// 钩子函数，从ff_ffplay.c中执行
cv::Mat PanoramaRenderer::frame;
std::mutex PanoramaRenderer::textureMutex;
panorama::DualFisheyeSticher initializeSticher();
panorama::DualFisheyeSticher sticher = initializeSticher();

panorama::DualFisheyeSticher initializeSticher() {
    // 360 全景拼接初始化,下面参数适合insta360 设备的
    panorama::cameraParam cam1, cam2;
    cv::Size outputSize = cv::Size(2000, 1000);
    float hemisphereWidth = 960.0f;                                                       //OBS推流是960.0f
    cam1.circleFisheyeImage = cv::Mat::zeros(hemisphereWidth, hemisphereWidth, CV_8UC3);  // 前单个球
    cam1.FOV = 189.2357;
    cam1.centerPt = cv::Point2f(hemisphereWidth / 2.0, hemisphereWidth / 2.0);
    cam1.radius = hemisphereWidth / 2.0;
    cam1.rotateX = 0.01112242;
    cam1.rotateY = 0.2971962;
    cam1.rotateZ = -0.0007757799;

    // cam2
    cam2.circleFisheyeImage = cv::Mat::zeros(hemisphereWidth, hemisphereWidth, CV_8UC3);  // 后单个球;
    cam2.FOV = 194.1712;
    cam2.centerPt = cv::Point2f(hemisphereWidth / 2.0, hemisphereWidth / 2.0);
    cam2.radius = hemisphereWidth / 2.0;
    cam2.rotateX = -0.7172632;
    cam2.rotateY = 0.5694329;
    cam2.rotateZ = 179.9732;
    panorama::DualFisheyeSticher sticher = panorama::DualFisheyeSticher(cam1, cam2, outputSize);
    return sticher;
}

void processDecodedFrame(AVFrame *avFrame) {
    PanoramaRenderer::processDecodedFrameImpl(avFrame);
}

void PanoramaRenderer::processUI(cv::Mat &matFrame) {
    // Lock the textureMutex and update the frame
    std::lock_guard<std::mutex> lock(textureMutex);
    {
        cv::Mat img;
        cv::cvtColor(matFrame, img, cv::COLOR_BGR2RGB);
        cv::flip(img, img, 0);
        frame = img.clone();

        // 把frame中位于左右2个半球的鱼眼转换为equirectangular类型全景图
        cv::Mat frontFrame = frame(cv::Rect(0, 0, frame.cols / 2, frame.rows));
        cv::Mat backFrame = frame(cv::Rect(frame.cols / 2, 0, frame.cols / 2, frame.rows));
        frame = sticher.stich(frontFrame, backFrame);
    }
}

void PanoramaRenderer::processDecodedFrameImpl(AVFrame *avFrame) {
    // Check if the frame is in a format OpenCV understands
    if (avFrame->format != AV_PIX_FMT_BGR24 && avFrame->format != AV_PIX_FMT_RGB24) {
        // Convert frame to BGR24 or RGB24 format using sws_scale
        struct SwsContext *img_convert_ctx = sws_getContext(
            avFrame->width, avFrame->height, (AVPixelFormat)avFrame->format,
            avFrame->width, avFrame->height, AV_PIX_FMT_BGR24, SWS_BICUBIC,
            nullptr, nullptr, nullptr);

        AVFrame *pFrameBGR = av_frame_alloc();
        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, avFrame->width, avFrame->height, 1);
        uint8_t *buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

        av_image_fill_arrays(pFrameBGR->data, pFrameBGR->linesize, buffer,
                             AV_PIX_FMT_BGR24, avFrame->width, avFrame->height, 1);

        sws_scale(img_convert_ctx, avFrame->data, avFrame->linesize, 0, avFrame->height,
                  pFrameBGR->data, pFrameBGR->linesize);

        // Now pFrameBGR contains the BGR24 formatted data.
        // Convert this frame to OpenCV Mat
        cv::Mat img(cv::Size(avFrame->width, avFrame->height), CV_8UC3, pFrameBGR->data[0], pFrameBGR->linesize[0]);
        {
            // Lock the textureMutex and update the frame
            std::lock_guard<std::mutex> lock(textureMutex);
            cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
            cv::flip(img, img, 0);
            frame = img.clone();

            // 把frame中位于左右2个半球的鱼眼转换为equirectangular类型全景图
            cv::Mat frontFrame = frame(cv::Rect(0, 0, frame.cols / 2, frame.rows));
            cv::Mat backFrame = frame(cv::Rect(frame.cols / 2, 0, frame.cols / 2, frame.rows));
            frame = sticher.stich(frontFrame, backFrame);
            //    cv::imwrite(sharePath+"/dst_stich_front.jpg",frontFrame);
            //    cv::imwrite(sharePath+"/dst_stich_back.jpg",backFrame);
            //    cv::imwrite(sharePath+"/dst_stich.jpg",frame);
        }

        // Free resources
        av_free(buffer);
        av_frame_free(&pFrameBGR);
        sws_freeContext(img_convert_ctx);
    } else {
        // If the frame is already in BGR24 or RGB24 format, directly create the Mat
        cv::Mat img(cv::Size(avFrame->width, avFrame->height), CV_8UC3, avFrame->data[0], avFrame->linesize[0]);
        {
            // Lock the textureMutex and update the frame
            std::lock_guard<std::mutex> lock(textureMutex);
            cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
            cv::flip(img, img, 0);
            frame = img.clone();

            cv::Mat frontFrame = frame(cv::Rect(0, 0, frame.cols / 2, frame.rows));
            cv::Mat backFrame = frame(cv::Rect(frame.cols / 2, 0, frame.cols / 2, frame.rows));
            frame = sticher.stich(frontFrame, backFrame);
        }
    }
}

PanoramaRenderer::PanoramaRenderer(AAssetManager *assetManager, std::string filepath)
    : shaderProgram(0), texture(0), videoTexture(0), vboVertices(0), vboTexCoords(0), vboIndices(0), sphereData(new SphereData(1.0f, 50, 50)), assetManager(assetManager), sharePath(std::move(filepath)), rotationX(0.0f), rotationY(0.0f), zoom(1.0f), widthScreen(800), heightScreen(800), ahrs(1.0f / 60.0f), viewOrientation(ViewMode::LITTLEPLANET), gyroOpen(GyroMode::GYRODISABLED), panoMode(SwitchMode::PANORAMAIMAGE), view(glm::mat4(1.0)), gyroMat(glm::mat4(1.0)) {
    // Open the input file
    //std::string mp4File = sharePath+"/360panorama.mp4"; // 360panorama.mp4
    //videoCapture.open(mp4File);

    if (viewOrientation == PanoramaRenderer::ViewMode::PERSPECTIVE) {
        zoom = 1;
    } else if (viewOrientation == PanoramaRenderer::ViewMode::LITTLEPLANET) {
        zoom = 2;
    } else if (viewOrientation == PanoramaRenderer::ViewMode::CRYSTALBALL) {
        zoom = 2;
    } else {
        zoom = 1;
    }
}

PanoramaRenderer::~PanoramaRenderer() {
    delete sphereData;
    glDeleteProgram(shaderProgram);
    glDeleteTextures(1, &texture);
    glDeleteTextures(1, &videoTexture);
    glDeleteBuffers(1, &vboVertices);
    glDeleteBuffers(1, &vboTexCoords);
    glDeleteBuffers(1, &vboIndices);
    glDeleteVertexArrays(1, &vao);
}

GLuint PanoramaRenderer::loadShader(GLenum type, const char *shaderSrc) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderSrc, nullptr);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char *infoLog = new char[infoLen];
            glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
            LOGE("Error compiling shader:\n%s\n", infoLog);
            delete[] infoLog;
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint PanoramaRenderer::createProgram(const char *vertexSrc, const char *fragmentSrc) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSrc);
    if (!vertexShader) {
        LOGE("Failed to load vertex shader");
        return 0;
    }

    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSrc);
    if (!fragmentShader) {
        LOGE("Failed to load fragment shader");
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char *infoLog = new char[infoLen];
            glGetProgramInfoLog(program, infoLen, nullptr, infoLog);
            LOGE("Error linking program:\n%s\n", infoLog);
            delete[] infoLog;
        }
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

// 全景图像需要的函数
GLuint PanoramaRenderer::loadTexture(const char *imagePath) {
    int width, height, nrChannels;
    cv::Mat img = cv::imread(imagePath, cv::IMREAD_COLOR);
    if (img.empty()) {
        LOGE("Failed to load texture image from asset:%s", imagePath);
        return 0;
    }

    // Convert the image to RGB format if it is BGR
    if (img.channels() == 3) {
        cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
    }
    // Flip the image vertically
    cv::flip(img, img, 0);
    width = img.cols;
    height = img.rows;

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texture;
}

void PanoramaRenderer::onSurfaceCreated() {
    LOGI("onSurfaceCreated have successfully Initialized.\n");
    const char *vertexShaderSource =
        "#version 300 es\n"
        "layout(location = 0) in vec3 aPos;\n"
        "layout(location = 1) in vec2 aTexCoord;\n"
        "out vec2 TexCoord;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "void main()\n"
        "{\n"
        "   TexCoord = aTexCoord;\n"
        "   gl_Position = projection * view * vec4(aPos, 1.0);\n"
        "}\n";

    const char *fragmentShaderSource =
        "#version 300 es\n"
        "precision mediump float;\n"
        "in vec2 TexCoord;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D texture1;\n"
        "void main()\n"
        "{\n"
        "FragColor = texture(texture1, TexCoord);\n"
        "}\n";

    shaderProgram = createProgram(vertexShaderSource, fragmentShaderSource);
    if (!shaderProgram) {
        LOGE("Failed to create shader program");
        return;
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboVertices);
    glGenBuffers(1, &vboIndices);
    glGenBuffers(1, &vboTexCoords);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
    glBufferData(GL_ARRAY_BUFFER, sphereData->getNumVertices() * sizeof(GLfloat), sphereData->getVertices(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);  // 第5个参数stride也可以设置为0来让OpenGL决定具体步长是多少（只有当数值是紧密排列时才可用）
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, vboTexCoords);
    glBufferData(GL_ARRAY_BUFFER, sphereData->getNumTexs() * sizeof(GLfloat), sphereData->getTexCoords(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereData->getNumIndices() * sizeof(GLushort), sphereData->getIndices(), GL_STATIC_DRAW);

    if (panoMode == SwitchMode::PANORAMAIMAGE) {
        std::string imagePath = sharePath + "/360panorama.jpg";
        texture = loadTexture(imagePath.c_str());
        if (!texture) {
            LOGE("Failed to load texture");
            return;
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);  // 解绑 VBO,360全景图像最好需要
        glBindVertexArray(0);              // 解绑VAO,360全景图像最好需要
        glGenerateMipmap(GL_TEXTURE_2D);   //全景图像使用，但是视频渲染不使用 glGenerateMipmap,较少性能开销
    } else if (panoMode == SwitchMode::PANORAMAVIDEO) {
        glGenTextures(1, &videoTexture);
        glBindTexture(GL_TEXTURE_2D, videoTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frameWidth, frameHeight,
                     0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    // 启用深度测试，防止遮挡影响
    glEnable(GL_DEPTH_TEST);
    // 设置深度测试函数
    glDepthFunc(GL_LESS);
    LOGI("onSurfaceCreated have successfully run.\n");
}

//void PanoramaRenderer::videoDecodingLoop() {
//////////////////////////之前使用OpenCV解码视频播放/////////////////////////////////////
//    while (!stopPlayback) {
//        cv::Mat tempFrame;
//        if (!videoCapture.read(tempFrame)) {
//            LOGE("Failed to read frame from video");
//            break;
//        }
//
//        cv::flip(tempFrame,frame,0);
//        //cv::cvtColor(tempFrame, tempFrame, cv::COLOR_BGR2RGB);
//        // std::this_thread::sleep_for(std::chrono::milliseconds(30)); // Adjust frame rate if needed
//        {
//            std::lock_guard<std::mutex> lock(textureMutex);
//            if (!frame.empty()) {
//                frameReady = true;
//                frameReadyCondition.notify_one();  // Notify the rendering thread
//                LOGI("videoDecodingLoop: buffer copied and notified.");
//            }
//        }
//    }
/////////////////////end of 之前使用OpenCV解码视频播放/////////////////////////////////////
// LOGI("videoDecodingLoop has successfully run.\n");
//}

void PanoramaRenderer::onDrawFrame() {
    //    LOGI("onDrawFrame have successfully initialized.\n");
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    float fovDeg = 70 * zoom;
    if (fovDeg < 50)
        fovDeg = 50;
    if (fovDeg > 145)
        fovDeg = 145;

    glm::vec3 cameraPos, target, upVector;
    glm::vec3 viewDir, upDir;
    if (viewOrientation == PanoramaRenderer::ViewMode::PERSPECTIVE) {
        cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);  // 摄像机位置
        target = glm::vec3(0.0f, 0.0f, -1.0f);    // 目标点
        upVector = glm::vec3(0.0f, 1.0f, 0.0f);   // 上向量
    } else if (viewOrientation == PanoramaRenderer::ViewMode::LITTLEPLANET) {
        cameraPos = glm::vec3(0.0f, 1.0f, 0.0f);
        target = glm::vec3(0.0f, 0.0f, 0.0f);
        upVector = glm::vec3(-1.0f, 0.0f, 0.0f);
    } else if (viewOrientation == PanoramaRenderer::ViewMode::CRYSTALBALL) {
        cameraPos = glm::vec3(0.0f, -1.2f, 0.0f);
        target = glm::vec3(0.0f, 0.0f, 0.0f);
        upVector = glm::vec3(-1.0f, 1.0f, 0.0f);
    } else {
        cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
        target = glm::vec3(0.0f, 0.0f, -1.0f);
        upVector = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    //    if (gyroOpen == PanoramaRenderer::GyroMode::GYROENABLED) {
    //        cameraPos = glm::vec3(gyroMat*glm::vec4(cameraPos,1.0));
    //        target = glm::vec3(gyroMat * glm::vec4(target, 1.0));
    //        upVector = glm::vec3(gyroMat * glm::vec4(upVector, 0.0));
    //
    ////        glm::vec3 temp = glm::vec3(glm::vec4(1.0f,2.0f,3.0f,4.0f));
    ////        LOGI("temp:%.f,%.f,%.f",temp[0],temp[1],temp[2]);
    //        LOGI("target:%.0f,%.0f,%.0f,upVector:%.0f,%.0f,%.0f",target[0],target[1],target[2],
    //             upVector[0],upVector[1],upVector[2]);
    //    }

    viewDir = glm::normalize(target - cameraPos);
    upDir = glm::normalize(upVector);

    // 生成视图矩阵
    view = glm::lookAt(cameraPos, target, upVector);
    if (gyroOpen == PanoramaRenderer::GyroMode::GYROENABLED) {
        view = view * gyroMat;
    }

    if (gyroOpen == PanoramaRenderer::GyroMode::GYRODISABLED) {
        glm::vec3 rotateAxis = glm::normalize(glm::cross(viewDir, upDir));
        view = glm::rotate(view, glm::radians(rotationY), rotateAxis);  // 绕cameraPos中心旋转
        view = glm::rotate(view, glm::radians(rotationX), upDir);
    }

    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");

    projection = glm::perspective(glm::radians(fovDeg), (float)widthScreen / (float)heightScreen, 0.1f, 100.0f);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    glBindVertexArray(vao);

    if (panoMode == SwitchMode::PANORAMAIMAGE) {
        glActiveTexture(GL_TEXTURE0);           // 全景图像使用
        glBindTexture(GL_TEXTURE_2D, texture);  // 全景图像使用
    } else if (panoMode == SwitchMode::PANORAMAVIDEO) {
        updateVideoFrame();
        std::unique_lock<std::mutex> lock(textureMutex);
        if (!frame.empty()) {
            //cv::imwrite(sharePath + "/dst_ondrawFrame.jpg", frame);
            glBindTexture(GL_TEXTURE_2D, videoTexture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.cols, frame.rows, GL_RGB, GL_UNSIGNED_BYTE, frame.data);
        }
    }

    glDrawElements(GL_TRIANGLES, sphereData->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);  // Unbind VAO
    //    LOGI("onDrawFrame have successfully run.\n");
}

void PanoramaRenderer::onSurfaceChanged(int width, int height) {
    //    LOGI("onSurfaceChanged have successfully initialized.\n");
    widthScreen = width;
    heightScreen = height;
    glViewport(0, 0, width, height);
    //    LOGI("onSurfaceChanged have successfully run.\n");
}

void PanoramaRenderer::handleTouchDrag(float deltaX, float deltaY) {
    rotationX += deltaX * 0.1f;
    rotationY += deltaY * 0.1f;
}

void PanoramaRenderer::handlePinchZoom(float scaleFactor) {
    zoom *= scaleFactor;
}

void PanoramaRenderer::setSwitchMode(SwitchMode mode) {
    panoMode = mode;
}

void PanoramaRenderer::setViewMode(ViewMode mode) {
    viewOrientation = mode;
}

void PanoramaRenderer::setGyroMode(GyroMode mode) {
    gyroOpen = mode;
}

void PanoramaRenderer::onGyroAccUpdate(float gyroX, float gyroY, float gyroZ, float accX, float accY, float accZ) {
    glm::vec3 gyro(gyroX, gyroY, gyroZ);  // Gyroscope values in rad/s
    glm::vec3 acc(accX, accY, accZ);      // Accelerometer values in m/s²
}

void PanoramaRenderer::onQuaternionUpdate(float quatW, float quatX, float quatY, float quatZ,
                                          float accX, float accY, float accZ) {
    // 参考https://source.android.com/docs/core/interaction/sensors/sensor-types?hl=zh-cn#accelerometer
    // https://developer.android.com/reference/android/hardware/SensorEvent#values

    // 创建四元数,为ENU世界坐标系下的设备绝对位姿
    glm::quat deviceOrientation(quatW, quatX, quatY, quatZ);
    //    glm::vec3 gravity = glm::vec3(accX,accY,accZ);

    //    glm::vec3 euler = glm::degrees(glm::eulerAngles(deviceOrientation));
    //    //LOGI("quaternion:%.2f,%.2f,%.2f,%.2f,accelerometer:%.2f,%.2f,%.2f",quatW,quatX,quatY,quatZ,accX,accY,accZ);
    //    LOGI("euler:%.2f,%.2f,%.2f",euler.x,euler.y,euler.z);
    //
    //    // 重力方向的四元数（将重力方向标准化）
    //    glm::vec3 gravityNormalized = glm::normalize(gravity);
    //    glm::quat gravityOrientation = glm::quatLookAt(gravityNormalized, glm::vec3(0, 1,0));
    //
    //    // 计算最终的四元数
    //    glm::quat finalOrientation = gravityOrientation * deviceOrientation;

    // 将四元数转换为旋转矩阵
    glm::mat4 rotationMatrix = glm::mat4_cast(deviceOrientation);
    rotationMatrix = glm::transpose(rotationMatrix);
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // 更新陀螺仪矩阵
    gyroMat = rotationMatrix;
}

// Create external texture for video frames
GLuint PanoramaRenderer::createExternalTexture() {
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);

    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Unbind texture
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

    return textureId;
}

// Update video frame
void PanoramaRenderer::updateVideoFrame() {
    std::lock_guard<std::mutex> lock(textureMutex);  // Lock for thread safety
    if (frame.empty()) {
        LOGI("frame is empty1!");
        return;  // No frame to update
    }

    // Update the OpenGL texture with the current video frame
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, videoTexture);

    // Upload frame data to texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame.cols, frame.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, frame.data);

    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind texture
}

// JNI Interfaces
extern "C" {
JNIEXPORT jlong JNICALL
Java_com_example_my360panorama_MainActivity_00024PanoramaRenderer_nativeCreateRenderer(JNIEnv *env, jobject obj, jobject assetManager, jstring path) {
    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
    // Returns a pointer to an array of bytes representing the string
    // in modified UTF-8 encoding. This array is valid until it is released
    // by ReleaseStringUTFChars().
    const char *temp = env->GetStringUTFChars(path, nullptr);
    return reinterpret_cast<jlong>(new PanoramaRenderer(mgr, std::string(temp)));
}

JNIEXPORT jint JNICALL
Java_com_example_my360panorama_MainActivity_00024PanoramaRenderer_nativeCreateExternalTexture(JNIEnv *env, jobject instance, jlong rendererPtr) {
    // Cast the renderer pointer to your PanoramaRenderer class
    PanoramaRenderer *renderer = reinterpret_cast<PanoramaRenderer *>(rendererPtr);
    // Call the createExternalTexture function and return the texture ID
    return renderer->createExternalTexture();
}

JNIEXPORT void JNICALL
Java_com_example_my360panorama_MainActivity_00024PanoramaRenderer_nativeOnDrawFrame(JNIEnv *env, jobject instance, jlong rendererPtr) {
    // Cast the renderer pointer to your PanoramaRenderer class
    PanoramaRenderer *renderer = reinterpret_cast<PanoramaRenderer *>(rendererPtr);

    // Call the onDrawFrame function which should include the call to updateVideoFrame()
    renderer->onDrawFrame();
}

JNIEXPORT void JNICALL
Java_com_example_my360panorama_MainActivity_00024PanoramaRenderer_nativeOnSurfaceCreated(JNIEnv *env, jobject obj, jlong rendererPtr) {
    PanoramaRenderer *renderer = reinterpret_cast<PanoramaRenderer *>(rendererPtr);
    renderer->onSurfaceCreated();
}

JNIEXPORT void JNICALL
Java_com_example_my360panorama_MainActivity_00024PanoramaRenderer_nativeOnSurfaceChanged(JNIEnv *env, jobject obj, jlong rendererPtr, jint width, jint height) {
    PanoramaRenderer *renderer = reinterpret_cast<PanoramaRenderer *>(rendererPtr);
    renderer->onSurfaceChanged(width, height);
}

JNIEXPORT void JNICALL
Java_com_example_my360panorama_MainActivity_00024PanoramaRenderer_nativeHandleTouchDrag(JNIEnv *env, jobject obj, jlong rendererPtr, jfloat deltaX, jfloat deltaY) {
    PanoramaRenderer *renderer = reinterpret_cast<PanoramaRenderer *>(rendererPtr);
    renderer->handleTouchDrag(deltaX, deltaY);
}

JNIEXPORT void JNICALL
Java_com_example_my360panorama_MainActivity_00024PanoramaRenderer_nativeHandlePinchZoom(JNIEnv *env, jobject obj, jlong rendererPtr, jfloat scaleFactor) {
    PanoramaRenderer *renderer = reinterpret_cast<PanoramaRenderer *>(rendererPtr);
    renderer->handlePinchZoom(scaleFactor);
}

// 处理从 Java 传递来的陀螺仪数据
JNIEXPORT void JNICALL
Java_com_example_my360panorama_MainActivity_00024PanoramaRenderer_nativeOnGyroAccUpdate(JNIEnv *env, jobject obj, jlong rendererPtr, jfloat gyroX, jfloat gyroY, jfloat gyroZ,
                                                                                        jfloat accX, jfloat accY, jfloat accZ) {
    PanoramaRenderer *renderer = reinterpret_cast<PanoramaRenderer *>(rendererPtr);
    if (renderer != nullptr) {
        renderer->onGyroAccUpdate(gyroX, gyroY, gyroZ, accX, accY, accZ);
    }
}

// 处理从 Java 传递来的陀螺仪数据，直接调用JAVA已经融合好的四元数
JNIEXPORT void JNICALL
Java_com_example_my360panorama_MainActivity_00024PanoramaRenderer_nativeOnGameRotationUpdate(JNIEnv *env, jobject obj, jlong rendererPtr, jfloat quatW, jfloat quatX, jfloat quatY,
                                                                                             jfloat quatZ, jfloat accX, jfloat accY, jfloat accZ) {
    PanoramaRenderer *renderer = reinterpret_cast<PanoramaRenderer *>(rendererPtr);
    if (renderer != nullptr) {
        renderer->onQuaternionUpdate(quatW, quatX, quatY, quatZ, accX, accY, accZ);
    }
}
}
