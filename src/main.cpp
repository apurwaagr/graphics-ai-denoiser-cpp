#include "core/denoiser.hpp"
#include "core/renderer.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>

namespace {

int parseOrDefault(const char *value, int fallback) {
    try {
        return std::stoi(value);
    } catch (...) {
        return fallback;
    }
}

float psnr(const gfx::Image &a, const gfx::Image &b) {
    if (a.width != b.width || a.height != b.height || a.pixels.empty()) {
        return 0.0f;
    }

    double mse = 0.0;
    for (size_t i = 0; i < a.pixels.size(); ++i) {
        const float dr = a.pixels[i].r - b.pixels[i].r;
        const float dg = a.pixels[i].g - b.pixels[i].g;
        const float db = a.pixels[i].b - b.pixels[i].b;
        mse += static_cast<double>(dr * dr + dg * dg + db * db);
    }

    mse /= static_cast<double>(a.pixels.size() * 3);
    if (mse <= 1e-12) {
        return std::numeric_limits<float>::infinity();
    }

    return static_cast<float>(10.0 * std::log10(1.0 / mse));
}

}

int main(int argc, char **argv) {
    int width = 512;
    int height = 512;
    int spp = 4;
    int threads = 0;
    int denoiseRadius = 2;
    int benchmarkRefMultiplier = 8;
    uint32_t seed = 42U;
    bool waitAtEnd = false;
    bool benchmark = false;

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
        const std::string arg = argv[i];
        if (arg == "--wait") {
            waitAtEnd = true;
        } else if (arg == "--benchmark") {
            benchmark = true;
        } else if (arg == "--threads" && i + 1 < argc) {
            threads = parseOrDefault(argv[++i], threads);
        } else if (arg == "--radius" && i + 1 < argc) {
            denoiseRadius = parseOrDefault(argv[++i], denoiseRadius);
        } else if (arg == "--ref-mult" && i + 1 < argc) {
            benchmarkRefMultiplier = parseOrDefault(argv[++i], benchmarkRefMultiplier);
        } else if (arg == "--seed" && i + 1 < argc) {
            seed = static_cast<uint32_t>(parseOrDefault(argv[++i], static_cast<int>(seed)));
        }
    }

    if (width <= 0 || height <= 0 || spp <= 0 || denoiseRadius < 0 || benchmarkRefMultiplier < 1) {
        std::cerr << "Invalid args. Usage: graphics_ai_demo [width height spp]"
                  << " [--threads N] [--radius N] [--seed N] [--benchmark] [--ref-mult N] [--wait]"
                  << std::endl;
        return 1;
    }

    gfx::Renderer renderer;
    gfx::EdgeAwareDenoiser denoiser;

    std::cout << "Starting render: " << width << "x" << height
              << ", spp=" << spp
              << ", threads=" << ((threads > 0) ? threads : 0)
              << ", radius=" << denoiseRadius
              << ", seed=" << seed
              << std::endl;

    const auto t0 = std::chrono::high_resolution_clock::now();
    const auto noisy = renderer.renderNoisy(width, height, spp, seed, threads);
    const auto t1 = std::chrono::high_resolution_clock::now();

    const auto denoised = denoiser.denoise(noisy, denoiseRadius, 1.2f, 0.08f, threads);
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

    if (benchmark) {
        const int referenceSpp = std::max(1, spp * benchmarkRefMultiplier);
        std::cout << "Benchmark mode: rendering reference frame with spp=" << referenceSpp << std::endl;
        const auto b0 = std::chrono::high_resolution_clock::now();
        const auto reference = renderer.renderNoisy(width, height, referenceSpp, seed, threads);
        const auto b1 = std::chrono::high_resolution_clock::now();
        const float noisyPsnr = psnr(noisy, reference);
        const float denoisedPsnr = psnr(denoised, reference);
        const auto referenceMs = std::chrono::duration_cast<std::chrono::milliseconds>(b1 - b0).count();

        std::cout << "Reference render: " << referenceMs << " ms" << std::endl;
        std::cout << "PSNR noisy      : " << noisyPsnr << " dB" << std::endl;
        std::cout << "PSNR denoised   : " << denoisedPsnr << " dB" << std::endl;
        std::cout << "Delta           : " << (denoisedPsnr - noisyPsnr) << " dB" << std::endl;
    }

    if (waitAtEnd) {
        std::cout << "Press Enter to close..." << std::endl;
        std::cin.get();
    }

    return 0;
}
