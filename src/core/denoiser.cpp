#include "core/denoiser.hpp"

#include <algorithm>
#include <cmath>
#include <thread>
#include <vector>

namespace gfx {

namespace {

inline int clampInt(int v, int lo, int hi) {
    return std::max(lo, std::min(hi, v));
}

inline float colorDistance2(const RGB &a, const RGB &b) {
    const float dr = a.r - b.r;
    const float dg = a.g - b.g;
    const float db = a.b - b.b;
    return dr * dr + dg * dg + db * db;
}

inline int resolveThreadCount(int requested, int maxWorkItems) {
    if (maxWorkItems <= 0) {
        return 1;
    }

    if (requested > 0) {
        return std::max(1, std::min(requested, maxWorkItems));
    }

    const unsigned int hw = std::thread::hardware_concurrency();
    const int fallback = (hw == 0U) ? 4 : static_cast<int>(hw);
    return std::max(1, std::min(fallback, maxWorkItems));
}

}  // namespace

Image EdgeAwareDenoiser::denoise(const Image &input,
                                 int radius,
                                 float sigmaSpatial,
                                 float sigmaRange,
                                 int threadCount) const {
    Image output(input.width, input.height);

    const float spatialFactor = -0.5f / (sigmaSpatial * sigmaSpatial);
    const float rangeFactor = -0.5f / (sigmaRange * sigmaRange);
    const int workers = resolveThreadCount(threadCount, input.height);
    const int rowsPerWorker = (input.height + workers - 1) / workers;

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(workers));

    for (int worker = 0; worker < workers; ++worker) {
        const int yStart = worker * rowsPerWorker;
        const int yEnd = std::min(input.height, yStart + rowsPerWorker);
        if (yStart >= yEnd) {
            continue;
        }

        threads.emplace_back([&, yStart, yEnd]() {
            for (int y = yStart; y < yEnd; ++y) {
                for (int x = 0; x < input.width; ++x) {
                    const RGB center = input.at(x, y);

                    float sumW = 0.0f;
                    float sumR = 0.0f;
                    float sumG = 0.0f;
                    float sumB = 0.0f;

                    for (int dy = -radius; dy <= radius; ++dy) {
                        for (int dx = -radius; dx <= radius; ++dx) {
                            const int nx = clampInt(x + dx, 0, input.width - 1);
                            const int ny = clampInt(y + dy, 0, input.height - 1);

                            const RGB sample = input.at(nx, ny);
                            const float ds2 = static_cast<float>(dx * dx + dy * dy);
                            const float dr2 = colorDistance2(center, sample);
                            const float w = std::exp(ds2 * spatialFactor + dr2 * rangeFactor);

                            sumW += w;
                            sumR += w * sample.r;
                            sumG += w * sample.g;
                            sumB += w * sample.b;
                        }
                    }

                    if (sumW > 0.0f) {
                        output.at(x, y) = RGB{sumR / sumW, sumG / sumW, sumB / sumW};
                    } else {
                        output.at(x, y) = center;
                    }
                }
            }
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    return output;
}

}  // namespace gfx
