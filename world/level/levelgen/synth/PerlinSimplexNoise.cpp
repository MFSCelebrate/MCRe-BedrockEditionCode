#include <cstdint>
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>

// ---------- 基础向量 ----------
struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float x_, float y_) : x(x_), y(y_) {}
};

struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

// ---------- Random 占位 ----------
// 实际应使用 Minecraft Bedrock 的 Random，这里仅提供接口
struct Random {
    Random() {}
    Random(unsigned int seed) { /* 初始化 MT19937 等 */ }
    float nextFloat() { return 0.0f; } // 返回 [0,1)
    int nextInt(int bound) { return 0; } // 返回 [0,bound)
};

// ---------- ImprovedNoise 简化版 ----------
// 与原版 ImprovedNoise 接口一致，用于 PerlinSimplexNoise
class ImprovedNoise {
public:
    float xOffset, yOffset, zOffset; // 0x0, 0x4, 0x8
    int p[512];                      // 0xc 开始，共 512 个 int

    ImprovedNoise() {
        xOffset = yOffset = zOffset = 0.0f;
        for (int i = 0; i < 512; ++i) p[i] = 0;
    }

    ImprovedNoise(Random& random) {
        xOffset = yOffset = zOffset = 0.0f;
        _init(random);
    }

    void _init(Random& random) {
        xOffset = random.nextFloat() * 256.0f;
        yOffset = random.nextFloat() * 256.0f;
        zOffset = random.nextFloat() * 256.0f;

        for (int i = 0; i < 256; ++i) p[i] = i;
        for (int i = 0; i < 256; ++i) {
            int j = random.nextInt(256 - i) + i;
            std::swap(p[i], p[j]);
            p[i + 256] = p[i];
        }
    }

    static float fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    static int floorf_i(float v) {
        int i = static_cast<int>(v);
        if (static_cast<float>(i) > v) --i;
        return i;
    }

    static float grad(int hash, float x, float y, float z) {
        int h = hash & 15;
        float u = (h < 8) ? x : y;
        float v;
        if (h < 4) v = y;
        else if (h == 12 || h == 14) v = x;
        else v = z;
        float r = 0.0f;
        r += ((h & 1) == 0) ? u : -u;
        r += ((h & 2) == 0) ? v : -v;
        return r;
    }

    float _noise(const Vec3& v) const {
        float x = v.x + xOffset;
        float y = v.y + yOffset;
        float z = v.z + zOffset;

        int ix = floorf_i(x);
        int iy = floorf_i(y);
        int iz = floorf_i(z);

        float fx = x - static_cast<float>(ix);
        float fy = y - static_cast<float>(iy);
        float fz = z - static_cast<float>(iz);

        float u = fade(fx);
        float vv = fade(fy);
        float w = fade(fz);

        int A  = p[ix & 0xff] + iy;
        int B  = p[(ix + 1) & 0xff] + iy;
        int AA = p[A & 0xff] + iz;
        int AB = p[(A + 1) & 0xff] + iz;
        int BA = p[B & 0xff] + iz;
        int BB = p[(B + 1) & 0xff] + iz;

        float g000 = grad(p[AA & 0xff], fx, fy, fz);
        float g100 = grad(p[BA & 0xff], fx - 1.0f, fy, fz);
        float c00 = g000 + u * (g100 - g000);

        float g010 = grad(p[AB & 0xff], fx, fy - 1.0f, fz);
        float g110 = grad(p[BB & 0xff], fx - 1.0f, fy - 1.0f, fz);
        float c10 = g010 + u * (g110 - g010);

        float g001 = grad(p[(AA + 1) & 0xff], fx, fy, fz - 1.0f);
        float g101 = grad(p[(BA + 1) & 0xff], fx - 1.0f, fy, fz - 1.0f);
        float c01 = g001 + u * (g101 - g001);

        float g011 = grad(p[(AB + 1) & 0xff], fx, fy - 1.0f, fz - 1.0f);
        float g111 = grad(p[(BB + 1) & 0xff], fx - 1.0f, fy - 1.0f, fz - 1.0f);
        float c11 = g011 + u * (g111 - g011);

        float y0 = c00 + vv * (c10 - c00);
        float y1 = c01 + vv * (c11 - c01);
        return y0 + w * (y1 - y0);
    }
};

// ---------- PerlinSimplexNoise ----------
class PerlinSimplexNoise {
public:
    // 成员布局（根据反汇编推断）：
    int octaves;                        // 0x0
    std::vector<ImprovedNoise> noises;  // 0x4, 0x8, 0xc (vector)
    float normalization;                // 0x10

    PerlinSimplexNoise();
    PerlinSimplexNoise(unsigned int seed, int octaves);
    PerlinSimplexNoise(Random& random, int octaves);
    ~PerlinSimplexNoise();

    void _init(Random& random);

    float getValue(float x, float y) const;
    float getValueNormalized(float x, float y) const;
    float getValue(const Vec3& pos) const;
    float getValueNormalized(const Vec3& pos) const;

    void getRegion(float* out, const Vec2& origin,
                   int sizeX, int sizeY,
                   const Vec2& scale, float amplitude, float frequency) const;
    void getRegion(float* out, const Vec3& origin,
                   int sizeX, int sizeY, int sizeZ,
                   const Vec3& scale) const;

    int hashCode() const;
};

// 默认构造函数
PerlinSimplexNoise::PerlinSimplexNoise()
    : octaves(0), normalization(1.0f) {
    noises.clear();
}

// 析构函数
PerlinSimplexNoise::~PerlinSimplexNoise() {
    // vector 自动释放
}

// 构造函数：PerlinSimplexNoise(unsigned int seed, int octaves)
PerlinSimplexNoise::PerlinSimplexNoise(unsigned int seed, int octaves)
    : octaves(octaves), normalization(1.0f) {
    noises.clear();
    Random random(seed);
    _init(random);
}

// 构造函数：PerlinSimplexNoise(Random& random, int octaves)
PerlinSimplexNoise::PerlinSimplexNoise(Random& random, int octaves)
    : octaves(octaves), normalization(1.0f) {
    noises.clear();
    _init(random);
}

// 初始化
void PerlinSimplexNoise::_init(Random& random) {
    noises.clear();
    if (octaves <= 0) {
        normalization = 1.0f;
        return;
    }

    noises.reserve(octaves);

    float totalWeight = 0.0f;
    float weight = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        noises.emplace_back(random); // 构造 ImprovedNoise(random)
        totalWeight += weight;
        weight *= 0.5f;
    }

    normalization = 1.0f / totalWeight;
}

// 获取原始噪声值（2D）
float PerlinSimplexNoise::getValue(float x, float y) const {
    if (octaves <= 0 || noises.empty()) return 0.0f;

    float result = 0.0f;
    float weight = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        Vec3 pos(x * weight, y * weight, 0.0f);
        result += noises[i]._noise(pos);
        weight *= 0.5f;
    }
    return result;
}

// 获取归一化噪声值（2D）
float PerlinSimplexNoise::getValueNormalized(float x, float y) const {
    return getValue(x, y) * normalization;
}

// 获取原始噪声值（3D）
float PerlinSimplexNoise::getValue(const Vec3& pos) const {
    if (octaves <= 0 || noises.empty()) return 0.0f;

    float result = 0.0f;
    float weight = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        Vec3 p(pos.x * weight, pos.y * weight, pos.z * weight);
        result += noises[i]._noise(p);
        weight *= 0.5f;
    }
    return result;
}

// 获取归一化噪声值（3D）
float PerlinSimplexNoise::getValueNormalized(const Vec3& pos) const {
    return getValue(pos) * normalization;
}

// 二维区域采样
void PerlinSimplexNoise::getRegion(float* out, const Vec2& origin,
                                   int sizeX, int sizeY,
                                   const Vec2& scale, float amplitude, float frequency) const {
    // 清零输出
    std::memset(out, 0, sizeof(float) * sizeX * sizeY);

    if (octaves <= 0 || noises.empty()) return;

    // 原版使用 SIMD 和复杂权重，这里简化为逐点采样并累加。
    // frequency 和 amplitude 作为额外的缩放因子：frequency 缩放坐标，amplitude 缩放输出。
    for (int y = 0; y < sizeY; ++y) {
        for (int x = 0; x < sizeX; ++x) {
            Vec3 pos;
            pos.x = (origin.x + x * scale.x) * frequency;
            pos.y = (origin.y + y * scale.y) * frequency;
            pos.z = 0.0f;

            float value = getValueNormalized(pos) * amplitude;
            out[y * sizeX + x] += value;
        }
    }
}

// 三维区域采样
void PerlinSimplexNoise::getRegion(float* out, const Vec3& origin,
                                   int sizeX, int sizeY, int sizeZ,
                                   const Vec3& scale) const {
    // 清零输出
    std::memset(out, 0, sizeof(float) * sizeX * sizeY * sizeZ);

    if (octaves <= 0 || noises.empty()) return;

    for (int z = 0; z < sizeZ; ++z) {
        for (int y = 0; y < sizeY; ++y) {
            for (int x = 0; x < sizeX; ++x) {
                Vec3 pos;
                pos.x = origin.x + x * scale.x;
                pos.y = origin.y + y * scale.y;
                pos.z = origin.z + z * scale.z;

                float value = getValueNormalized(pos);
                int index = z * sizeX * sizeY + y * sizeX + x;
                out[index] += value;
            }
        }
    }
}

// 哈希码（参考 PerlinNoise 实现）
int PerlinSimplexNoise::hashCode() const {
    int h = 0x1267;
    for (const auto& noise : noises) {
        for (int i = 0; i < 512; ++i) {
            h = h * 37 + noise.p[i];
        }
    }
    return h;
}