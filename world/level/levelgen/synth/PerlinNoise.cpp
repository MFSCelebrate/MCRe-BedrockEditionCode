#include <cstdint>
#include <cmath>
#include <cstring>
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
struct Random {
    Random() {}
    Random(int seed) { /* ... */ }
    float nextFloat() { return 0.0f; } // 返回 [0,1)
    int nextInt(int bound) { return 0; }
};

// ---------- ImprovedNoise 简化版 ----------
// 原版 ImprovedNoise 在另一份反汇编中，这里只给出 PerlinNoise 需要的接口。
class ImprovedNoise {
public:
    float xOffset, yOffset, zOffset; // 0x0, 0x4, 0x8
    int p[512];                      // 0xc 开始，共 512 个 int

    ImprovedNoise() {
        xOffset = yOffset = zOffset = 0.0f;
        for (int i = 0; i < 512; ++i) p[i] = 0;
    }

    ImprovedNoise(Random& random) {
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

// ---------- PerlinNoise ----------
class PerlinNoise {
public:
    // 成员布局（根据反汇编推断）：
    int seed;                   // offset 0x0
    int octaves;                // offset 0x4
    ImprovedNoise* noises;      // offset 0x8  (begin)
    ImprovedNoise* noisesEnd;   // offset 0xc  (end)
    ImprovedNoise* noisesCap;   // offset 0x10 (capacity)
    float normalization;        // offset 0x14

    PerlinNoise();
    PerlinNoise(int seed);
    PerlinNoise(unsigned int seed, int octaves, int persistence);
    PerlinNoise(Random& random, int octaves, int persistence);
    ~PerlinNoise();

    void _init(Random& random);

    float getValue(const Vec3& pos) const;
    float getValueNormalized(const Vec3& pos) const;

    void getRegion(float* out, const Vec3& origin,
                   int sizeX, int sizeY, int sizeZ,
                   const Vec3& scale) const;

    void getRegion(float* out, const Vec2& origin,
                   int sizeX, int sizeY,
                   const Vec2& scale, float amplitude) const;

    int hashCode() const;
};

// ---------- 析构 ----------
PerlinNoise::~PerlinNoise() {
    if (noises) {
        // 原版会释放动态数组，这里简化为 delete[]
        delete[] noises;
        noises = nullptr;
        noisesEnd = nullptr;
        noisesCap = nullptr;
    }
}

// ---------- 构造函数：PerlinNoise(int) ----------
PerlinNoise::PerlinNoise(int seed) {
    this->seed = seed;
    this->octaves = 1;
    this->noises = nullptr;
    this->noisesEnd = nullptr;
    this->noisesCap = nullptr;
    this->normalization = 1.0f;

    // 原版使用 random_device 生成随机数，然后初始化一个 ImprovedNoise。
    // 这里等价为：用 seed 创建一个 Random，再构建一个 ImprovedNoise。
    Random random(seed);
    noises = new ImprovedNoise[1];
    noisesEnd = noises + 1;
    noisesCap = noisesEnd;
    noises[0] = ImprovedNoise(random);

    // 计算归一化因子（只有一个层时通常为 1.0f）
    normalization = 1.0f;
}

// ---------- 构造函数：PerlinNoise(unsigned int, int, int) ----------
PerlinNoise::PerlinNoise(unsigned int seed, int octaves, int persistence) {
    this->seed = static_cast<int>(seed);
    this->octaves = octaves;
    // persistence 在反汇编中未明显使用，可能用于权重递减
    this->noises = nullptr;
    this->noisesEnd = nullptr;
    this->noisesCap = nullptr;
    this->normalization = 1.0f;

    Random random(static_cast<int>(seed));
    _init(random);
}

// ---------- 构造函数：PerlinNoise(Random&, int, int) ----------
PerlinNoise::PerlinNoise(Random& random, int octaves, int persistence) {
    this->seed = 0;
    this->octaves = octaves;
    this->noises = nullptr;
    this->noisesEnd = nullptr;
    this->noisesCap = nullptr;
    this->normalization = 1.0f;

    _init(random);
}

// ---------- 初始化：_init(Random&) ----------
void PerlinNoise::_init(Random& random) {
    int n = octaves;
    if (n <= 0) {
        noises = nullptr;
        noisesEnd = nullptr;
        noisesCap = nullptr;
        normalization = 1.0f;
        return;
    }

    // 分配 ImprovedNoise 数组
    noises = new ImprovedNoise[n];
    noisesEnd = noises + n;
    noisesCap = noisesEnd;

    float totalWeight = 0.0f;
    float weight = 1.0f;
    for (int i = 0; i < n; ++i) {
        // 每个 ImprovedNoise 使用同一个 Random，但会消耗随机数
        noises[i] = ImprovedNoise(random);
        totalWeight += weight;
        weight *= 0.5f; // 默认 persistence = 0.5
    }

    normalization = 1.0f / totalWeight;
}

// ---------- 获取原始噪声值 ----------
float PerlinNoise::getValue(const Vec3& pos) const {
    if (octaves <= 0) return 0.0f;

    float result = 0.0f;
    float weight = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        result += noises[i]._noise(pos) * weight;
        weight *= 0.5f;
    }
    return result;
}

// ---------- 获取归一化噪声值 ----------
float PerlinNoise::getValueNormalized(const Vec3& pos) const {
    return getValue(pos) * normalization;
}

// ---------- 三维区域采样 ----------
void PerlinNoise::getRegion(float* out, const Vec3& origin,
                            int sizeX, int sizeY, int sizeZ,
                            const Vec3& scale) const {
    // 原版使用 SIMD 优化，这里给出等价的标量循环。
    // 输出数组会被累加（原版先 memset 0，然后累加）。
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

// ---------- 二维区域采样 ----------
void PerlinNoise::getRegion(float* out, const Vec2& origin,
                            int sizeX, int sizeY,
                            const Vec2& scale, float amplitude) const {
    // 原版对二维区域采样，内部可能固定 z = 0 或使用某个固定值。
    // 这里给出等价实现：对每个点计算噪声并乘以 amplitude。
    for (int y = 0; y < sizeY; ++y) {
        for (int x = 0; x < sizeX; ++x) {
            Vec3 pos;
            pos.x = origin.x + x * scale.x;
            pos.y = origin.y + y * scale.y;
            pos.z = 0.0f; // 二维情况通常固定 z

            float value = getValueNormalized(pos) * amplitude;
            int index = y * sizeX + x;
            out[index] += value;
        }
    }
}

// ---------- 哈希码 ----------
int PerlinNoise::hashCode() const {
    int h = 0x1267;
    for (int i = 0; i < octaves; ++i) {
        for (int j = 0; j < 512; ++j) {
            h = h * 37 + noises[i].p[j];
        }
    }
    return h;
}