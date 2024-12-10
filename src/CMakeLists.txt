# 编译全景渲染模块
add_executable(PanoViewer panorama_renderer.cpp Sphere.cpp) # 函数式编程
target_include_directories(PanoViewer PUBLIC ${GLEW_INCLUDE_PATH} ${GLFW_INCLUDE_DIR} ${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(PanoViewer ${GLEW_LIBRARY} ${GLFW_LIBRARY} ${OPENGL_LIBRARIES} ${OPENGL_LIBRARY} ${OpenCV_LIBS})

add_executable(360Viewer main.cpp PanoramaRenderer.cpp Sphere.cpp) # 面向对象编程
target_include_directories(360Viewer PUBLIC ${GLEW_INCLUDE_PATH} ${GLFW_INCLUDE_DIR} ${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(360Viewer ${GLEW_LIBRARY} ${GLFW_LIBRARY} ${OPENGL_LIBRARIES} ${OPENGL_LIBRARY} ${OpenCV_LIBS})