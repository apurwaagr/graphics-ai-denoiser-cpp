#include "core/denoiser.hpp"
#include "core/renderer.hpp"

#include <chrono>
#include <iostream>
#include <string>

namespace {

int parseOrDefault(const char *value, int fallback) {
    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

}

int main(int argc, char **argv) {
    int width = 512;
    int height = 512;
    int spp = 4;
    bool waitAtEnd = false;

    if (argc >= 2) {
        width = parseOrDefault(argv[1], width);
    }
    if (argc >= 3) {
        height = parseOrDefault(argv[2], height);
    }
    if (argc >= 4) {
        spp = parseOrDefault(argv[3], spp);
    }
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--wait") {
            waitAtEnd = true;
        }
    }

    if (width <= 0 || height <= 0 || spp <= 0) {
        std::cerr << "Invalid args. Usage: graphics_ai_demo [width height spp] [--wait]" << std::endl;
        return 1;
    }

    gfx::Renderer renderer;
    gfx::EdgeAwareDenoiser denoiser;

    std::cout << "Starting render: " << width << "x" << height << ", spp=" << spp << std::endl;

    const auto t0 = std::chrono::high_resolution_clock::now();
    const auto noisy = renderer.renderNoisy(width, height, spp);
    const auto t1 = std::chrono::high_resolution_clock::now();

    const auto denoised = denoiser.denoise(noisy, 2, 1.2f, 0.08f);
    const auto t2 = std::chrono::high_resolution_clock::now();

    const bool wroteNoisy = renderer.writePPM(noisy, "output_noisy.ppm");
    const bool wroteDenoised = renderer.writePPM(denoised, "output_denoised.ppm");
    const auto t3 = std::chrono::high_resolution_clock::now();

    if (!wroteNoisy || !wroteDenoised) {
        std::cerr << "Failed to write one or more output images." << std::endl;
        return 1;
    }

    const auto renderMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    const auto denoiseMs = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    const auto writeMs = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count();
    const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t0).count();

    std::cout << "Render stage   : " << renderMs << " ms" << std::endl;
    std::cout << "Denoise stage  : " << denoiseMs << " ms" << std::endl;
    std::cout << "Write stage    : " << writeMs << " ms" << std::endl;
    std::cout << "Total          : " << totalMs << " ms" << std::endl;
    std::cout << "Rendered output_noisy.ppm and output_denoised.ppm" << std::endl;

    if (waitAtEnd) {
        std::cout << "Press Enter to close..." << std::endl;
        std::cin.get();
    }

    return 0;
}
