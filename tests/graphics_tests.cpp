#include <iostream>
#include <stdexcept>
#include "../src/graphics.hpp"

using namespace graphics;
constexpr uint32_t white = 0xffffffffu;
constexpr uint32_t black = 0xff000000u;
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
uint32_t at(int x, int y) { return pixels[y * WIDTH + x]; }

int main() {
    try {
        WIDTH = HEIGHT = 64;
        pixels.assign(WIDTH * HEIGHT, white);
        initFont();
        drawLine(-1000000, 32, 1000000, 32, black);
        require(std::count(pixels.begin(), pixels.end(), black) == 64, "Clipped horizontal line");
        clearBuffer(white);
        drawLine(5, 5, 20, 20, black);
        require(at(5,5) == black && at(20,20) == black && at(12,12) == black, "Line endpoints and diagonal");
        clearBuffer(white);
        drawLine(-5, -5, -1, -1, black);
        require(std::count(pixels.begin(), pixels.end(), black) == 0, "Fully offscreen line");
        drawCircle(32, 32, 1, black);
        require(at(33,32) == black && at(31,32) == black && at(32,33) == black && at(32,31) == black, "Radius-one circle cardinal points");
        clearBuffer(white);
        drawCircle(32,32,10,black);
        for (int y = 22; y <= 42; ++y) for (int x = 22; x <= 42; ++x) {
            require(at(x,y) == at(64-x,y) && at(x,y) == at(x,64-y), "Circle symmetry");
        }
        clearBuffer(white);
        solidFill({{5,5},{30,5},{30,15},{15,15},{15,30},{5,30}}, black);
        require(at(10,25) == black && at(25,10) == black && at(25,25) == white, "Concave polygon interior and notch");
        clearBuffer(white);
        solidFill({{-1000000,-1000000},{1000000,-1000000},{1000000,1000000},{-1000000,1000000}}, black);
        require(std::count(pixels.begin(), pixels.end(), black) == 4096, "Offscreen fill clipped to viewport");
        clearBuffer(white);
        scanlineFill({{0,0},{40,0},{40,28},{0,28}}, black);
        require(at(1,0) == black && at(0,0) == white && at(21,0) == black, "Pattern repeats with transparent background");
        clearBuffer(white);
        drawBezier({5,40},{15,5},{45,60},{58,10},black);
        require(at(5,40) == black && at(58,10) == black, "Bezier endpoint interpolation");
        Cube cube;
        const auto initial = cube.v;
        cube.rotate(0,0,2*PI);
        for (size_t i = 0; i < initial.size(); ++i) {
            require(std::abs(initial[i].x-cube.v[i].x) < 1e-8 && std::abs(initial[i].y-cube.v[i].y) < 1e-8, "Full cube rotation");
        }
        clearBuffer(white);
        cube.translate(0,0,-1000);
        cube.draw(500);
        require(std::count(pixels.begin(), pixels.end(), white) == 4096, "Cube behind camera is skipped");
        std::cout << "PASS: line clipping, circle symmetry, concave fill, pattern, Bezier, cube transforms\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n'; return 1;
    }
}
