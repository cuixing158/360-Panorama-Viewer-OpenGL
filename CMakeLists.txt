cmake_minimum_required(VERSION 3.5.0) 
project(PanoViewer)

# look for modules in this dir
# message(STATUS "CMAKE_MODULE_PATH原始路径：${CMAKE_MODULE_PATH}，CMAKE_SOURCE_DIR：${CMAKE_SOURCE_DIR}")
# set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}" ${CMAKE_MODULE_PATH} )
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
message(STATUS "CMAKE_MODULE_PATH现在路径：${CMAKE_MODULE_PATH}")

FIND_PACKAGE(OpenGL REQUIRED)
IF(MSVC)
  FIND_PACKAGE(GLEWW REQUIRED)
ELSE(MSVC)
  FIND_PACKAGE(GLEWW REQUIRED)
ENDIF(MSVC)

FIND_PACKAGE(GLFW REQUIRED)

message(STATUS "GLFW_LIBRARY:${GLFW_LIBRARY}")

IF(MSVC)
   SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ox /Ob2 /GS- /MP /fp:fast")
   # no expensive range checking
   ADD_DEFINITIONS(-D_SECURE_SCL=0)
   ADD_DEFINITIONS(-D_ITERATOR_DEBUG_LEVEL=0)
   ADD_DEFINITIONS(-DUNICODE)
   ADD_DEFINITIONS(-D_UNICODE)
   ADD_DEFINITIONS(-DGLEW_STATIC)
      
   # suppress MSVC security warnings
   ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ELSE(MSVC)
   add_definitions(-std=c++0x -Wall -g)
ENDIF(MSVC)

if(UNIX)
  find_package(X11 REQUIRED)
  find_package(Threads REQUIRED)
endif(UNIX)

  # includes
  INCLUDE_DIRECTORIES( "./src/" )
  INCLUDE_DIRECTORIES( ${GLEW_INCLUDE_PATH} )
  INCLUDE_DIRECTORIES( ${GLFW_INCLUDE_DIR} )

  # libjpeg
  ADD_SUBDIRECTORY(libjpeg)
  INCLUDE_DIRECTORIES("./libjpeg")
  include_directories("${CMAKE_CURRENT_BINARY_DIR}/libjpeg")

  source_group(glew FILES ${GLEW_SRC_FILE})

SET(SRC_PANOVIEWER 
  src/camera.h
  src/glfont.h src/glfont.cpp
  src/image.h src/imgjpg.h src/quaterniont.h src/vec3t.h
  src/pnm.h src/pnm.cpp src/glutil.h src/glutil.cpp
  src/TiledImage.h src/TiledImage.cpp
  src/main.cpp
  )
SOURCE_GROUP(PanoViewer FILES ${SRC_PANOVIEWER})

# WIN32
ADD_EXECUTABLE(PanoViewer 
  ${SRC_PANOVIEWER})
target_include_directories(PanoViewer PUBLIC ${GLEW_INCLUDE_PATH})
target_link_libraries(PanoViewer ${GLEW_LIBRARY} ${GLFW_LIBRARY})

SET(WINLIBS winmm.lib)
SET(LIBRARIES ${OPENGL_LIBRARIES} ${OPENGL_LIBRARY} ${GLEW_LIBRARY} ${X11_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

IF(MSVC)
  TARGET_LINK_LIBRARIES(PanoViewer ${LIBRARIES} ${WINLIBS} libjpeg)
ELSE(MSVC)
  TARGET_LINK_LIBRARIES(PanoViewer ${LIBRARIES} libjpeg)
ENDIF(MSVC)



# 使用自己编写的全景渲染模块
set(OpenCV_DIR "E:/softwares/MinGW64_v8_OpenCV4_4_Contrib_install")
find_package(OpenCV REQUIRED)
add_executable(myPanoViewer src2/panorama_renderer.cpp src2/Sphere.cpp)
target_include_directories(myPanoViewer PUBLIC ${GLEW_INCLUDE_PATH} ${GLFW_INCLUDE_DIR} ${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} src2)
message(STATUS "OpenCV_LIBS:${OpenCV_LIBS}")
target_link_libraries(myPanoViewer ${GLEW_LIBRARY} ${GLFW_LIBRARY} ${OPENGL_LIBRARIES} ${OPENGL_LIBRARY} ${OpenCV_LIBS})