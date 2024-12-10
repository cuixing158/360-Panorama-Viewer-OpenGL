
#include <iostream>
#include "PanoramaRenderer.h"
int main(int argc, char* argv[]) {
    if (argc == 1 || (argc == 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help"))) {
        std::cout << " Usage: " << argv[0] << " [filepath] [-h|--help]" << std::endl;
        std::cout << "  filepath: Path to the panorama image or video file." << std::endl;
        std::cout << "  -h, --help: Show this help message." << std::endl;
        std::cout << "  Drag the mouse to adjust the viewing direction, use the scroll wheel to zoom the FOV, and keys 1, 2, and 3 represent the perspective view, asteroid, and crystal ball respectively." << std::endl;
        return 0;
    } else if (argc == 2) {
        std::string filepath = argv[1];
        PanoramaRenderer renderer(filepath);
        // 进入渲染循环等操作
        renderer.renderLoop();
    } else {
        std::cerr << "Invalid number of arguments." << std::endl;
        std::cout << "Usage: " << argv[0] << " [filepath] [-h|--help]" << std::endl;
        return 1;
    }
    return 0;
}
