# PanoViewer

a small panoramic image viewer for equirectangular projections written
in C++ using OpenGL. Projection is done directly in the fragment shader
on the GPU, and it uses a tiling method to be able to load large images
that exceed the maximum size of a texture on the graphics card.

## Notes

崔星星记录

把第三方的panoviewer项目<https://github.com/pwrobel-logitech/panoramic>加入到了本项目,但是该项目使用的是SDL2作为图形库，对于桌面360 VR项目的渲染，如果只考虑渲染和性能，GLFW是更推荐的选择，能够提供更好的控制和性能。另一方面，如果需要配合音频、复杂输入和其他多媒体功能，SDL可能会更合适。

## Dependencies

PanoViewer depends on GLEW <http://glew.sourceforge.net/> （适用于桌面应用程序，不适用安卓studio项目中使用）and GLFW 3 <http://www.glfw.org/> (适用于桌面应用程序，不适用安卓studio项目中使用)for platform independent OpenGL and libjpeg <http://www.ijg.org/> for loading jpg's.

Libjpeg is statically included in the code, just put the content of
jpegsr<x>.zip from the IJG in the "libjpeg" subdirectory before running
CMake

## Usage

drag and rop any .jpg file on the panoviewer window to load the image

mouse:

- hold LMB and move : drag screen
- hold RMB and move : scroll in direction

keyboard:

- H to toggle on screen help text
- W,S to rotate view up/down
- Q,E to de/increase field of view
- C toggle compatibility render mode (shaders on/off)
- SPACE toggle on-screen text

## References

1. <https://github.com/ulikk/panoviewer>
1. <https://github.com/pwrobel-logitech/panoramic>
1. <https://github.com/Martin20150405/Pano360>
