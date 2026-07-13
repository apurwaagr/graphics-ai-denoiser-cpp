#include "core/denoiser.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {

float luma(const gfx::RGB &c) {
    return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
}

}  // namespace

int main() {
    gfx::Image img(8, 8);

    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            const float base = (x < 4) ? 0.2f : 0.8f;
            const float checker = ((x + y) % 2 == 0) ? 0.08f : -0.08f;
            const float v = std::fmax(0.0f, std::fmin(1.0f, base + checker));
            img.at(x, y) = gfx::RGB{v, v, v};
        }
    }

    gfx::EdgeAwareDenoiser denoiser;
    const auto out = denoiser.denoise(img, 1, 1.0f, 0.12f);

    const float beforeContrast = std::fabs(luma(img.at(3, 4)) - luma(img.at(4, 4)));
    const float afterContrast = std::fabs(luma(out.at(3, 4)) - luma(out.at(4, 4)));

    const float beforeNoise = std::fabs(luma(img.at(1, 1)) - luma(img.at(1, 2)));
    const float afterNoise = std::fabs(luma(out.at(1, 1)) - luma(out.at(1, 2)));

    assert(afterNoise < beforeNoise);
    assert(afterContrast > 0.35f * beforeContrast);

    std::cout << "denoiser_test passed" << std::endl;
    return 0;
}
