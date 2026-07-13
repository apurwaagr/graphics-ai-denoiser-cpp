#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gfx {

struct RGB {
    float r;
    float g;
    float b;
};

struct Image {
    int width;
    int height;
    std::vector<RGB> pixels;

    Image(int w, int h);
    RGB &at(int x, int y);
    const RGB &at(int x, int y) const;
};

class Renderer {
public:
    Image renderNoisy(int width, int height, int spp, uint32_t seed = 42U, int threadCount = 0) const;
    bool writePPM(const Image &image, const std::string &path) const;
};

}  // namespace gfx
