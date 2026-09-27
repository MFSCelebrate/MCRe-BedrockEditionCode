#include <cmath>
#include <cstdint>
#include <algorithm>

struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

// Random 仅作占位，原版为 Minecraft Bedrock 的 Random
struct Random {
    Random() {}
    Random(int seed) { /* ... */ }

    float nextFloat() {
        // 返回 [0, 1)
        return 0.0f;
    }

    int nextInt(int bound) {
        // 返回 [0, bound)
        return 0;
    }
};

class ImprovedNoise {
public:
    // 偏移：对应 this + 0x0 / 0x4 / 0x8
    float xOffset;
    float yOffset;
    float zOffset;

    // 置换表：对应 this + 0xc，实际为 int32_t p[512]
    int p[512];

    ImprovedNoise();
    ImprovedNoise(Random& random);

    void _init(Random& random);

    float _noise(const Vec3& v) const;

    void _blendCubeCorners(
        const Vec3& f,
        int ix,
        int iy,
        int iz,
        float u,
        float& c00,
        float& c10,
        float& c01,
        float& c11
    ) const;

    float _lerp(float t, float a, float b) const;

    float _grad2(int hash, float x, float y) const;
    float _grad(int hash, const Vec3& v) const;

    float _getValue(const Vec3& v) const;

    void _readArea(
        float* out,
        const Vec3& origin,
        int sx,
        int sy,
        int sz,
        const Vec3& scale,
        float amplitude
    ) const;

    int _hashCode() const;

    // 原函数实际返回小数部分，并通过引用输出 fade 值
    float _calcValues(float value, int& i, float& fadeOut) const;

private:
    static float fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    static int floorf_i(float v) {
        int i = static_cast<int>(v);
        if (static_cast<float>(i) > v) {
            --i;
        }
        return i;
    }

    // 标准 Perlin 梯度函数，等价于原版 16 方向梯度查表
    static float grad(int hash, float x, float y, float z) {
        int h = hash & 15;
        float u = (h < 8) ? x : y;
        float v;
        if (h < 4) {
            v = y;
        } else if (h == 12 || h == 14) {
            v = x;
        } else {
            v = z;
        }

        float r = 0.0f;
        r += ((h & 1) == 0) ? u : -u;
        r += ((h & 2) == 0) ? v : -v;
        return r;
    }
};

// ===== ImprovedNoise::ImprovedNoise() =====
ImprovedNoise::ImprovedNoise() {
    xOffset = 0.0f;
    yOffset = 0.0f;
    zOffset = 0.0f;

    // 原版此处构造默认 Random，种子类似 1，然后调用 _init
    Random random(1);
    _init(random);
}

// ===== ImprovedNoise::ImprovedNoise(Random&) =====
ImprovedNoise::ImprovedNoise(Random& random) {
    xOffset = 0.0f;
    yOffset = 0.0f;
    zOffset = 0.0f;

    _init(random);
}

// ===== ImprovedNoise::_init(Random&) =====
void ImprovedNoise::_init(Random& random) {
    // 对应三次 nextFloat() * 256.0f
    xOffset = random.nextFloat() * 256.0f;
    yOffset = random.nextFloat() * 256.0f;
    zOffset = random.nextFloat() * 256.0f;

    // 初始化 p[0..255] = 0..255
    for (int i = 0; i < 256; ++i) {
        p[i] = i;
    }

    // Fisher-Yates shuffle，并复制到 p[256..511]
    for (int i = 0; i < 256; ++i) {
        int j = random.nextInt(256 - i) + i;
        std::swap(p[i], p[j]);
        p[i + 256] = p[i];
    }
}

// ===== ImprovedNoise::_lerp(float, float, float) =====
float ImprovedNoise::_lerp(float t, float a, float b) const {
    return a + t * (b - a);
}

// ===== ImprovedNoise::_grad(int, Vec3 const&) =====
float ImprovedNoise::_grad(int hash, const Vec3& v) const {
    return grad(hash, v.x, v.y, v.z);
}

// ===== ImprovedNoise::_grad2(int, float, float) =====
float ImprovedNoise::_grad2(int hash, float x, float y) const {
    // 二维梯度，等价于 z = 0 的 _grad
    return grad(hash, x, y, 0.0f);
}

// ===== ImprovedNoise::_calcValues(float, int&, float&) =====
// 原函数：计算 floor、小数部分、fade 值。
// 返回值：小数部分 f；i 输出整数部分低 8 位；fadeOut 输出 fade(f)。
float ImprovedNoise::_calcValues(float value, int& i, float& fadeOut) const {
    int fi = floorf_i(value);
    i = fi & 0xff;

    float f = value - static_cast<float>(fi);
    fadeOut = fade(f);

    return f;
}

// ===== ImprovedNoise::_blendCubeCorners(...) =====
// 计算立方体八个角点的梯度，并沿 x 方向用 u 插值，得到四个值。
void ImprovedNoise::_blendCubeCorners(
    const Vec3& f,
    int ix,
    int iy,
    int iz,
    float u,
    float& c00,
    float& c10,
    float& c01,
    float& c11
) const {
    int A  = p[ix & 0xff] + iy;
    int B  = p[(ix + 1) & 0xff] + iy;

    int AA = p[A & 0xff] + iz;
    int AB = p[(A + 1) & 0xff] + iz;
    int BA = p[B & 0xff] + iz;
    int BB = p[(B + 1) & 0xff] + iz;

    float x = f.x;
    float y = f.y;
    float z = f.z;

    // 000 与 100
    float g000 = grad(p[AA & 0xff], x, y, z);
    float g100 = grad(p[BA & 0xff], x - 1.0f, y, z);
    c00 = _lerp(u, g000, g100);

    // 010 与 110
    float g010 = grad(p[AB & 0xff], x, y - 1.0f, z);
    float g110 = grad(p[BB & 0xff], x - 1.0f, y - 1.0f, z);
    c10 = _lerp(u, g010, g110);

    // 001 与 101
    float g001 = grad(p[(AA + 1) & 0xff], x, y, z - 1.0f);
    float g101 = grad(p[(BA + 1) & 0xff], x - 1.0f, y, z - 1.0f);
    c01 = _lerp(u, g001, g101);

    // 011 与 111
    float g011 = grad(p[(AB + 1) & 0xff], x, y - 1.0f, z - 1.0f);
    float g111 = grad(p[(BB + 1) & 0xff], x - 1.0f, y - 1.0f, z - 1.0f);
    c11 = _lerp(u, g011, g111);
}

// ===== ImprovedNoise::_noise(Vec3 const&) const =====
float ImprovedNoise::_noise(const Vec3& v) const {
    // 加上偏移
    float x = v.x + xOffset;
    float y = v.y + yOffset;
    float z = v.z + zOffset;

    // floor
    int ix = floorf_i(x);
    int iy = floorf_i(y);
    int iz = floorf_i(z);

    // 小数部分
    float fx = x - static_cast<float>(ix);
    float fy = y - static_cast<float>(iy);
    float fz = z - static_cast<float>(iz);

    // fade
    float u = fade(fx);
    float vv = fade(fy);
    float w = fade(fz);

    // 立方体角点沿 x 插值后的四个值
    float c00, c10, c01, c11;
    Vec3 f(fx, fy, fz);

    _blendCubeCorners(f, ix, iy, iz, u, c00, c10, c01, c11);

    // 先沿 y 插值，再沿 z 插值
    float y0 = _lerp(vv, c00, c10);
    float y1 = _lerp(vv, c01, c11);
    return _lerp(w, y0, y1);
}

// ===== ImprovedNoise::_getValue(Vec3 const&) const =====
float ImprovedNoise::_getValue(const Vec3& v) const {
    // 原版通过一个跳转调用 _noise
    return _noise(v);
}

// ===== ImprovedNoise::_readArea(...) const =====
// 高度简化的等价逻辑：对区域中每个点采样并累加。
// 原版有大量 SIMD 优化和循环展开，但核心含义如下。
void ImprovedNoise::_readArea(
    float* out,
    const Vec3& origin,
    int sx,
    int sy,
    int sz,
    const Vec3& scale,
    float amplitude
) const {
    float invAmp = 1.0f / amplitude;

    for (int z = 0; z < sz; ++z) {
        for (int y = 0; y < sy; ++y) {
            for (int x = 0; x < sx; ++x) {
                Vec3 pos;
                pos.x = origin.x + static_cast<float>(x) * scale.x;
                pos.y = origin.y + static_cast<float>(y) * scale.y;
                pos.z = origin.z + static_cast<float>(z) * scale.z;

                float n = _getValue(pos);

                int idx = z * sx * sy + y * sx + x;
                out[idx] += n * invAmp;
            }
        }
    }
}

// ===== ImprovedNoise::_hashCode() const =====
int ImprovedNoise::_hashCode() const {
    int h = 0x1267;

    // 原版遍历 p[0..511]，每次 h = h * 37 + p[i]
    for (int i = 0; i < 512; ++i) {
        h = h * 37 + p[i];
    }

    return h;
}