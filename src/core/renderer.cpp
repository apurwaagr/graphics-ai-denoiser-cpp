#include "core/renderer.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <random>
#include <thread>
#include <vector>

namespace gfx {

namespace {

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Ray {
    Vec3 origin;
    Vec3 dir;
};

struct Sphere {
    Vec3 center;
    float radius;
    RGB albedo;
};

inline Vec3 makeVec3(float x, float y, float z) {
    return Vec3{x, y, z};
}

inline Vec3 add(const Vec3 &a, const Vec3 &b) {
    return makeVec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline Vec3 sub(const Vec3 &a, const Vec3 &b) {
    return makeVec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline Vec3 mul(const Vec3 &a, float s) {
    return makeVec3(a.x * s, a.y * s, a.z * s);
}

inline float dot(const Vec3 &a, const Vec3 &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float length(const Vec3 &v) {
    return std::sqrt(dot(v, v));
}

inline Vec3 normalize(const Vec3 &v) {
    const float len = length(v);
    if (len <= 1e-8f) {
        return makeVec3(0.0f, 1.0f, 0.0f);
    }
    return mul(v, 1.0f / len);
}

inline float clamp01(float v) {
    return std::max(0.0f, std::min(1.0f, v));
}

inline Vec3 randomUnitHemisphere(const Vec3 &normal, std::mt19937 &rng) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    const float u = dist(rng);
    const float v = dist(rng);
    const float phi = 2.0f * 3.1415926535f * u;
    const float z = v;
    const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    Vec3 local = makeVec3(r * std::cos(phi), z, r * std::sin(phi));

    Vec3 up = (std::fabs(normal.y) < 0.99f) ? makeVec3(0.0f, 1.0f, 0.0f) : makeVec3(1.0f, 0.0f, 0.0f);
    Vec3 tangent = normalize(makeVec3(
        normal.y * up.z - normal.z * up.y,
        normal.z * up.x - normal.x * up.z,
        normal.x * up.y - normal.y * up.x));
    Vec3 bitangent = normalize(makeVec3(
        normal.y * tangent.z - normal.z * tangent.y,
        normal.z * tangent.x - normal.x * tangent.z,
        normal.x * tangent.y - normal.y * tangent.x));

    Vec3 world = add(add(mul(tangent, local.x), mul(normal, local.y)), mul(bitangent, local.z));
    return normalize(world);
}

bool intersectSphere(const Ray &ray, const Sphere &s, float &tHit, Vec3 &nHit, RGB &albedoHit) {
    const Vec3 oc = sub(ray.origin, s.center);
    const float a = dot(ray.dir, ray.dir);
    const float b = 2.0f * dot(oc, ray.dir);
    const float c = dot(oc, oc) - s.radius * s.radius;
    const float disc = b * b - 4.0f * a * c;
    if (disc < 0.0f) {
        return false;
    }

    const float sqrtDisc = std::sqrt(disc);
    const float t0 = (-b - sqrtDisc) / (2.0f * a);
    const float t1 = (-b + sqrtDisc) / (2.0f * a);
    const float tCand = (t0 > 1e-3f) ? t0 : ((t1 > 1e-3f) ? t1 : -1.0f);
    if (tCand <= 0.0f) {
        return false;
    }

    tHit = tCand;
    const Vec3 p = add(ray.origin, mul(ray.dir, tHit));
    nHit = normalize(sub(p, s.center));
    albedoHit = s.albedo;
    return true;
}

RGB sampleSky(const Vec3 &dir) {
    const float t = 0.5f * (dir.y + 1.0f);
    return RGB{
        (1.0f - t) * 0.95f + t * 0.55f,
        (1.0f - t) * 0.97f + t * 0.72f,
        (1.0f - t) * 1.0f + t * 0.95f,
    };
}

RGB mulRgb(const RGB &a, const RGB &b) {
    return RGB{a.r * b.r, a.g * b.g, a.b * b.b};
}

RGB tracePath(Ray ray, const std::vector<Sphere> &scene, int maxBounces, std::mt19937 &rng) {
    RGB throughput{1.0f, 1.0f, 1.0f};
    RGB radiance{0.0f, 0.0f, 0.0f};

    for (int bounce = 0; bounce < maxBounces; ++bounce) {
        float closest = std::numeric_limits<float>::infinity();
        bool hit = false;
        Vec3 n{};
        RGB albedo{};

        for (const auto &s : scene) {
            float t = 0.0f;
            Vec3 nTmp{};
            RGB aTmp{};
            if (intersectSphere(ray, s, t, nTmp, aTmp) && t < closest) {
                closest = t;
                n = nTmp;
                albedo = aTmp;
                hit = true;
            }
        }

        if (!hit) {
            const RGB sky = sampleSky(ray.dir);
            radiance.r += throughput.r * sky.r;
            radiance.g += throughput.g * sky.g;
            radiance.b += throughput.b * sky.b;
            break;
        }

        const Vec3 hitPoint = add(ray.origin, mul(ray.dir, closest));
        ray.origin = add(hitPoint, mul(n, 1e-3f));
        ray.dir = randomUnitHemisphere(n, rng);
        throughput = mulRgb(throughput, albedo);
    }

    return radiance;
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

Image::Image(int w, int h) : width(w), height(h), pixels(static_cast<size_t>(w * h)) {}

RGB &Image::at(int x, int y) {
    return pixels[static_cast<size_t>(y * width + x)];
}

const RGB &Image::at(int x, int y) const {
    return pixels[static_cast<size_t>(y * width + x)];
}

Image Renderer::renderNoisy(int width, int height, int spp, uint32_t seed, int threadCount) const {
    Image img(width, height);

    const std::vector<Sphere> scene = {
        Sphere{makeVec3(0.0f, -1001.0f, 0.0f), 1000.0f, RGB{0.85f, 0.85f, 0.85f}},
        Sphere{makeVec3(-0.8f, -0.35f, -3.2f), 0.65f, RGB{0.86f, 0.23f, 0.24f}},
        Sphere{makeVec3(0.9f, -0.45f, -2.6f), 0.55f, RGB{0.21f, 0.45f, 0.88f}},
        Sphere{makeVec3(0.1f, 0.25f, -4.0f), 0.85f, RGB{0.22f, 0.82f, 0.48f}},
    };

    const Vec3 camPos = makeVec3(0.0f, 0.2f, 1.8f);
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    constexpr int kMaxBounces = 3;
    const int workers = resolveThreadCount(threadCount, height);
    const int rowsPerWorker = (height + workers - 1) / workers;

    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(workers));

    for (int worker = 0; worker < workers; ++worker) {
        const int yStart = worker * rowsPerWorker;
        const int yEnd = std::min(height, yStart + rowsPerWorker);
        if (yStart >= yEnd) {
            continue;
        }

        threads.emplace_back([&, worker, yStart, yEnd]() {
            std::mt19937 rng(seed + static_cast<uint32_t>(worker * 7919));
            std::uniform_real_distribution<float> jitter(0.0f, 1.0f);

            for (int y = yStart; y < yEnd; ++y) {
                for (int x = 0; x < width; ++x) {
                    float r = 0.0f;
                    float g = 0.0f;
                    float b = 0.0f;

                    for (int s = 0; s < spp; ++s) {
                        const float fx = (static_cast<float>(x) + jitter(rng)) / static_cast<float>(width);
                        const float fy = (static_cast<float>(y) + jitter(rng)) / static_cast<float>(height);

                        const float px = (2.0f * fx - 1.0f) * aspect;
                        const float py = (1.0f - 2.0f * fy);
                        Ray ray{camPos, normalize(makeVec3(px * 1.05f, py * 0.85f, -1.7f))};

                        const RGB c = tracePath(ray, scene, kMaxBounces, rng);
                        r += clamp01(c.r);
                        g += clamp01(c.g);
                        b += clamp01(c.b);
                    }

                    img.at(x, y) = RGB{r / spp, g / spp, b / spp};
                }
            }
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    return img;
}

bool Renderer::writePPM(const Image &image, const std::string &path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }

    out << "P6\n" << image.width << " " << image.height << "\n255\n";

    for (const auto &p : image.pixels) {
        const auto toByte = [](float v) -> unsigned char {
            return static_cast<unsigned char>(255.0f * clamp01(v));
        };

        const unsigned char rgb[3] = {toByte(p.r), toByte(p.g), toByte(p.b)};
        out.write(reinterpret_cast<const char *>(rgb), 3);
    }

    return true;
}

}  // namespace gfx
