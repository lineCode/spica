#include <iostream>

#include "../include/rainy.h"
using namespace rainy;

int main(int argc, char **argv) {
    std::cout << "Path tracing" << std::endl << std::endl;

    Renderer renderer(200, 200, 100, 5);
    renderer.render();

    return 0;
}
