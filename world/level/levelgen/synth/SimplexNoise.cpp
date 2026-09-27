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
// 实际应使用 Minecraft Bedrock 的 Random
struct Random {
    Random() {}
    Random(unsigned int seed) { /* ... */ }
    float nextFloat() { return 0.0f; }      // [0,1)
    double nextDouble() { return 0.0; }     // [0,1)
    int nextInt(int bound) { return 0; }    // [0,bound)
};

// ---------- SimplexNoise ----------
class SimplexNoise {
public:
    // 成员布局（根据反汇编推断）：
    float xOffset, yOffset, zOffset; // 0x0, 0x4, 0x8
    int p[512];                      // 0xc 开始，共 512 个 int

    SimplexNoise(Random& random);

    float _getValue(const Vec2& v) const;
    float _getValue(const Vec3& v) const;
    float _getValue(float x) const;

    void _add(float* out, float x, float y,
              int sizeX, int sizeY,
              float scaleX, float scaleY,
              float amplitude) const;

    void _add(float* out, const Vec3& origin,
              int sizeX, int sizeY, int sizeZ,
              const Vec3& scale, float amplitude) const;

private:
    static const float F2;
    static const float G2;
    static const float F3;
    static const float G3;
    static const int grad3[12][3];

    static float dot(const int g[3], float x, float y) {
        return g[0] * x + g[1] * y;
    }
    static float dot(const int g[3], float x, float y, float z) {
        return g[0] * x + g[1] * y + g[2] * z;
    }

    static int floorf_i(float v) {
        int i = static_cast<int>(v);
        if (static_cast<float>(i) > v) --i;
        return i;
    }
};

// ---------- 静态常量定义 ----------
const float SimplexNoise::F2 = 0.5f * (sqrtf(3.0f) - 1.0f);
const float SimplexNoise::G2 = (3.0f - sqrtf(3.0f)) / 6.0f;
const float SimplexNoise::F3 = 1.0f / 3.0f;
const float SimplexNoise::G3 = 1.0f / 6.0f;

const int SimplexNoise::grad3[12][3] = {
    {1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0},
    {1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1},
    {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}
};

// ---------- 构造函数 ----------
SimplexNoise::SimplexNoise(Random& random) {
    // 初始化三个偏移量
    xOffset = static_cast<float>(random.nextDouble() * 256.0);
    yOffset = static_cast<float>(random.nextDouble() * 256.0);
    zOffset = static_cast<float>(random.nextDouble() * 256.0);

    // 初始化 p[0..255] = 0..255
    for (int i = 0; i < 256; ++i) {
        p[i] = i;
    }

    // Fisher-Yates 洗牌，并复制到 p[256..511]
    for (int i = 0; i < 256; ++i) {
        int j = random.nextInt(256 - i) + i;
        std::swap(p[i], p[j]);
        p[i + 256] = p[i];
    }
}

// ---------- 2D Simplex 噪声 ----------
float SimplexNoise::_getValue(const Vec2& v) const {
    float x = v.x;
    float y = v.y;

    // 1. 坐标偏斜
    float s = (x + y) * F2;
    int i = floorf_i(x + s);
    int j = floorf_i(y + s);

    float t = (i + j) * G2;
    float X0 = i - t;
    float Y0 = j - t;
    float x0 = x - X0;
    float y0 = y - Y0;

    // 2. 确定两个中间顶点
    int i1, j1;
    if (x0 > y0) {
        i1 = 1; j1 = 0;
    } else {
        i1 = 0; j1 = 1;
    }

    float x1 = x0 - i1 + G2;
    float y1 = y0 - j1 + G2;
    float x2 = x0 - 1.0f + 2.0f * G2;
    float y2 = y0 - 1.0f + 2.0f * G2;

    // 3. 哈希得到三个角的梯度索引
    int ii = i & 255;
    int jj = j & 255;

    int gi0 = p[ii + p[jj]] % 12;
    int gi1 = p[ii + i1 + p[jj + j1]] % 12;
    int gi2 = p[ii + 1 + p[jj + 1]] % 12;

    // 4. 计算三个角的贡献
    float n0 = 0.0f;
    float t0 = 0.5f - x0 * x0 - y0 * y0;
    if (t0 >= 0) {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad3[gi0], x0, y0);
    }

    float n1 = 0.0f;
    float t1 = 0.5f - x1 * x1 - y1 * y1;
    if (t1 >= 0) {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad3[gi1], x1, y1);
    }

    float n2 = 0.0f;
    float t2 = 0.5f - x2 * x2 - y2 * y2;
    if (t2 >= 0) {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad3[gi2], x2, y2);
    }

    return 70.0f * (n0 + n1 + n2);
}

// ---------- 3D Simplex 噪声 ----------
float SimplexNoise::_getValue(const Vec3& v) const {
    float x = v.x;
    float y = v.y;
    float z = v.z;

    // 1. 坐标偏斜
    float s = (x + y + z) * F3;
    int i = floorf_i(x + s);
    int j = floorf_i(y + s);
    int k = floorf_i(z + s);

    float t = (i + j + k) * G3;
    float X0 = i - t;
    float Y0 = j - t;
    float Z0 = k - t;
    float x0 = x - X0;
    float y0 = y - Y0;
    float z0 = z - Z0;

    // 2. 确定三个中间顶点
    int i1, j1, k1;
    int i2, j2, k2;

    if (x0 >= y0) {
        if (y0 >= z0) {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
        } else if (x0 >= z0) {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1;
        } else {
            i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1;
        }
    } else {
        if (y0 < z0) {
            i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1;
        } else if (x0 < z0) {
            i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1;
        } else {
            i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
        }
    }

    float x1 = x0 - i1 + G3;
    float y1 = y0 - j1 + G3;
    float z1 = z0 - k1 + G3;
    float x2 = x0 - i2 + 2.0f * G3;
    float y2 = y0 - j2 + 2.0f * G3;
    float z2 = z0 - k2 + 2.0f * G3;
    float x3 = x0 - 1.0f + 3.0f * G3;
    float y3 = y0 - 1.0f + 3.0f * G3;
    float z3 = z0 - 1.0f + 3.0f * G3;

    // 3. 哈希得到四个角的梯度索引
    int ii = i & 255;
    int jj = j & 255;
    int kk = k & 255;

    int gi0 = p[ii + p[jj + p[kk]]] % 12;
    int gi1 = p[ii + i1 + p[jj + j1 + p[kk + k1]]] % 12;
    int gi2 = p[ii + i2 + p[jj + j2 + p[kk + k2]]] % 12;
    int gi3 = p[ii + 1 + p[jj + 1 + p[kk + 1]]] % 12;

    // 4. 计算四个角的贡献
    float n0 = 0.0f;
    float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 >= 0) {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad3[gi0], x0, y0, z0);
    }

    float n1 = 0.0f;
    float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 >= 0) {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad3[gi1], x1, y1, z1);
    }

    float n2 = 0.0f;
    float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 >= 0) {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad3[gi2], x2, y2, z2);
    }

    float n3 = 0.0f;
    float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 >= 0) {
        t3 *= t3;
        n3 = t3 * t3 * dot(grad3[gi3], x3, y3, z3);
    }

    return 32.0f * (n0 + n1 + n2 + n3);
}

// ---------- 1D Simplex 噪声 ----------
float SimplexNoise::_getValue(float x) const {
    int i = floorf_i(x);
    float f = x - i;
    int ii = i & 255;

    // 左侧贡献
    float t0 = 1.0f - f;
    float t0_2 = t0 * t0;
    float t0_4 = t0_2 * t0_2;
    int gi0 = p[ii] & 15;
    float n0 = t0_4 * (grad3[gi0][0] * f);

    // 右侧贡献
    float t1 = f;
    float t1_2 = t1 * t1;
    float t1_4 = t1_2 * t1_2;
    int gi1 = p[ii + 1] & 15;
    float n1 = t1_4 * (grad3[gi1][0] * (f - 1.0f));

    return 0.395f * (n0 + n1);
}

// ---------- 2D 区域累加 ----------
void SimplexNoise::_add(float* out, float x, float y,
                        int sizeX, int sizeY,
                        float scaleX, float scaleY,
                        float amplitude) const {
    for (int iy = 0; iy < sizeY; ++iy) {
        for (int ix = 0; ix < sizeX; ++ix) {
            float px = x + ix * scaleX;
            float py = y + iy * scaleY;
            float value = _getValue(Vec2(px, py)) * amplitude;
            out[iy * sizeX + ix] += value;
        }
    }
}

// ---------- 3D 区域累加 ----------
void SimplexNoise::_add(float* out, const Vec3& origin,
                        int sizeX, int sizeY, int sizeZ,
                        const Vec3& scale, float amplitude) const {
    for (int iz = 0; iz < sizeZ; ++iz) {
        for (int iy = 0; iy < sizeY; ++iy) {
            for (int ix = 0; ix < sizeX; ++ix) {
                Vec3 pos;
                pos.x = origin.x + ix * scale.x;
                pos.y = origin.y + iy * scale.y;
                pos.z = origin.z + iz * scale.z;
                float value = _getValue(pos) * amplitude;
                int index = iz * sizeX * sizeY + iy * sizeX + ix;
                out[index] += value;
            }
        }
    }
}