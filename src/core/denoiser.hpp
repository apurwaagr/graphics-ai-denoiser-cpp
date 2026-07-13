#pragma once

#include "core/renderer.hpp"

namespace gfx {

class EdgeAwareDenoiser {
public:
    Image denoise(const Image &input, int radius = 1, float sigmaSpatial = 1.0f, float sigmaRange = 0.1f) const;
};

}  // namespace gfx
