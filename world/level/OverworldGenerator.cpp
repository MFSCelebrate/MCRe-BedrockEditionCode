#include "OverworldGenerator.hpp"

// ---------------------------------------------------------------------------
// 反汇编里出现但没展开的功能组件的极简 stub。
// 真实项目里替换为对应的类。
// ---------------------------------------------------------------------------
class Dimension {
public:
    virtual ~Dimension() = default;
};
class Biome {};
class BlockVolume {};
class ChunkPos {
public:
    int x = 0;
    int z = 0;
};
class BiomeSource {};
class BlockSource {};
class BlockTickingQueue {};
class LevelChunk {};
class LevelData {};
class BiomeRegistry {};
class BiomeArea {};
class ChunkViewSource {};
class Random {};
class IFileAccess {};
class BlockPos { public: int x = 0, y = 0, z = 0; };
class BoundingBox {};

// ===========================================================================
// 构造 / 析构
// ===========================================================================

OverworldGenerator::OverworldGenerator(Dimension& dimension,
                                       unsigned int seed,
                                       bool         isClient,
                                       Biome const* defaultBiome)
    : m_isClient(isClient)
{
    // ---- 对应 block 1 ----
    //
    // 反汇编结构：
    //   1. 初始化基类两个 vtable
    //   2. 构造 +0x28 处的 0x10 字节对象（Random / seed 相关）
    //   3. 初始化 +0xd828 起的一批 schema 指针为 nullptr
    //   4. 用 pthread_key_create 初始化 TLS key（+0xd8d4）
    //   5. 调用一组 0x35ab1XX(子对象, seed) 形式的子构造
    //   6. 用 seed 做一次 xorshift 风格的 RNG 预热（while 循环）
    //   7. 分配并构造 8 个 0x18/0x14/0x10 字节的小对象，分别挂到 +0xd82c 起
    //   8. 用一组浮点常数填充 +0xd848 .. +0xd8a8
    //   9. 处理默认 biome 指针 +0xdea4

    // (2) +0x28 处对象
    // 反汇编对应 0x34aa3c0 调用，实际是构造一个 Random 并 seed 它
    m_random = reinterpret_cast<Random*>(::operator new(0x10));
    // TODO: 具体 Random 构造由项目里对应的类提供
    // new (m_random) Random(seed);

    // (5) 初始化一批 schema 指针为空
    m_schema_0xd828 = nullptr;
    m_schema_0xd82c = nullptr;
    m_schema_0xd830 = nullptr;
    m_schema_0xd834 = nullptr;
    m_schema_0xd838 = nullptr;
    m_schema_0xd83c = nullptr;
    m_schema_0xd840 = nullptr;
    m_schema_0xd844 = nullptr;

    // (6) seed 预热 —— 反汇编里的 while 循环用 0x6c078965 做乘法
    // 并对每个寄存器位做 xorshift。这里按语义重写成一个正常的 RNG 预热。
    {
        uint32_t s = seed;
        for (uint32_t i = 1; i < 0x18E; ++i) {
            uint32_t v = s;
            v ^= (v >> 30);
            v *= 0x6C078965u;
            v += i;
            s = v;
        }
    }

    // (8) 填充噪声参数：反汇编里逐个 divisor 调用 0x339d1e0（生成
    //     [0,1) 的随机 float），然后用一个固定常数除。这里按语义写成一组
    //     概率/权重赋值，具体数值需要按真实运行结果校准。
    //
    //     反汇编中的除数（十六进制 float 位模式）：
    //       0x41033333 ≈ 8.2
    //       0x40a66666 ≈ 5.2
    //       0x40866666 ≈ 4.2
    //       0x400ccccd ≈ 2.2
    //       0x3f99999a ≈ 1.2
    //       0x3e4ccccd ≈ 0.2
    //
    //     这里只做占位，具体映射到哪个 noise 参数需进一步核对。
    for (auto& f : m_noiseParams) {
        f = 0.0f;
    }

    // (9) 默认 biome 指针
    (void)defaultBiome;

    // 最后注册 features
    _registerFeatures();
}

OverworldGenerator::~OverworldGenerator() {
    // ---- 对应 block 3 ----
    // 反汇编做的是：把每个 schema 指针逐个 reset（判空 → release_weak），
    // 然后清理 +0x1c 处的多重继承基类。
    _destroyMembers();

    // 释放 m_random
    if (m_random) {
        // m_random->~Random();
        ::operator delete(m_random);
        m_random = nullptr;
    }
}

void OverworldGenerator::_destroyMembers() {
    // 反汇编里对每个 schema 指针都做：
    //   if (ptr) { ptr->vtable[8](); shared_weak_count::__release_weak(ptr); }
    // 也就是释放一个 shared_ptr。
    //
    // 这里统一用一个宏处理。
    auto resetPtr = [](void*& p) {
        if (!p) return;
        // 实际是 std::shared_ptr 的 reset：
        //   auto* ctrl = p;
        //   ctrl->vtable[8]();
        //   std::__ndk1::__shared_weak_count::__release_weak(ctrl);
        p = nullptr;
    };

    resetPtr(m_schema_0xd828);
    resetPtr(m_schema_0xd82c);
    resetPtr(m_schema_0xd830);
    resetPtr(m_schema_0xd834);
    resetPtr(m_schema_0xd838);
    resetPtr(m_schema_0xd83c);
    resetPtr(m_schema_0xd840);
    resetPtr(m_schema_0xd844);

    resetPtr(m_schema_0xd8b0);
    resetPtr(m_schema_0xd8c0);
    resetPtr(m_schema_0xd8c8);
    resetPtr(m_schema_0xd8cc);
    resetPtr(m_schema_0xd8d0);
    resetPtr(m_schema_0xd8d4);

    resetPtr(m_featureRegistry_0xd8e0);
    resetPtr(m_structureRegistry_0xd95c);
    resetPtr(m_structureRegistry_0xda00);
    resetPtr(m_structureRegistry_0xda7c);
    resetPtr(m_structureRegistry_0xdae4);
    resetPtr(m_structureRegistry_0xdaf0);
    resetPtr(m_structureRegistry_0xdb78);
    resetPtr(m_structureRegistry_0xdbfc);
    resetPtr(m_structureRegistry_0xdc7c);
    resetPtr(m_structureRegistry_0xdcfc);
    resetPtr(m_structureRegistry_0xdd74);

    resetPtr(m_featureList_0xddf0);
    resetPtr(m_featureList_0xddf8);
    resetPtr(m_featureList_0xde00);
    resetPtr(m_featureList_0xde08);
    resetPtr(m_fileAccess);
    resetPtr(m_callbackCapture);
}

// ===========================================================================
// 生成管线
// ===========================================================================

void OverworldGenerator::_registerFeatures() {
    // ---- 对应 block 1 里从 +0xd828 起的大段注册 ----
    //
    // 反汇编里对每个 feature 都是：
    //   1. new(size) + 构造
    //   2. 把返回的指针挂到对应的 +0xd8XX 字段
    //   3. 若旧字段非空则释放
    //
    // 从 +0xd828 到 +0xd844 一共 8 个，后面还有一批。
    //
    // 每个 feature 都持有一个 std::function<void(...)> 作为回调。
    // 由于我们无法从反汇编恢复每个 feature 的具体语义（Mineshaft、
    // Village、Ravine、Temple 等），下面只把结构写清楚，内容留空。
    //
    // TODO: 按真实 feature 名字逐个填。

    // 对应 0x34a9700 调用（size=0x18/0x14/0x10/0x8）
    // 这是一个泛化的“按 size 分配 + 构造 + 赋值 + 释放旧值”的模板实例化。
    auto allocAndAssign = [](void*& slot, size_t size) {
        void* fresh = ::operator new(size);
        // TODO: 真实构造
        if (slot) {
            // reset(slot)
        }
        slot = fresh;
    };

    allocAndAssign(m_schema_0xd828, 0x18);
    allocAndAssign(m_schema_0xd82c, 0x18);
    allocAndAssign(m_schema_0xd830, 0x18);
    allocAndAssign(m_schema_0xd834, 0x14);
    allocAndAssign(m_schema_0xd838, 0x18);
    allocAndAssign(m_schema_0xd83c, 0x18);
    allocAndAssign(m_schema_0xd840, 0x18);
    allocAndAssign(m_schema_0xd844, 0x14);

    // 后面 0xd8e0 / 0xd95c / 0xda00 / 0xda7c / 0xdae4 / 0xdaf0 / 0xdb78 /
    // 0xdbfc / 0xdc7c / 0xdcfc / 0xdd74 / 0xddf0 / 0xddf8 / 0xde00 / 0xde08
    // 同理由项目里对应的类填充。
}

// ---------------------------------------------------------------------------
// prepareHeights 公开入口
// ---------------------------------------------------------------------------
void OverworldGenerator::prepareHeights(BlockVolume& volume,
                                        const ChunkPos& pos,
                                        bool isClient)
{
    // ---- 对应 block 5 ----
    // 反汇编：构造一个临时 std::function，然后调用 _prepareHeights。
    // 简化版本：
    //   std::function<void(...)> tmp = bind(...);
    //   _prepareHeights(volume, pos, *biomeSource, isClient);
    //
    // BiomeSource 从 this 的成员里取（反汇编里通过 +0xde8c 那个 function 拿到）。
    //
    // 这里给出结构，具体 source 由项目决定：
    //
    // auto& biomeSource = this->getBiomeSource();
    // _prepareHeights(volume, pos, biomeSource, isClient);

    (void)volume; (void)pos; (void)isClient;
    // TODO: 补上从成员取 biomeSource 的逻辑
}

// ---------------------------------------------------------------------------
// _prepareHeights
//
// 核心：给定 ChunkPos，对 16x16 的 (x,z) 网格，每 4x4 一组采样 5x5
// BiomeArea，然后用噪声合成高度场。
//
// 反汇编里这块是双层循环 + 大量 SSE 噪音插值，逐行照抄无意义。
// 这里恢复顶层结构：
// ---------------------------------------------------------------------------
void OverworldGenerator::_prepareHeights(BlockVolume& volume,
                                         const ChunkPos& pos,
                                         const BiomeSource& biomeSource,
                                         bool isClient)
{
    (void)biomeSource;

    // 反汇编里先对 425 个 float 做一次 memset
    std::array<float, 425> noiseBuffer{};

    // 5x5 的格子，步长 4
    constexpr int kGridSize = 5;
    constexpr int kStep     = 4;

    // X/Z 遍历 4x4 大格
    for (int gz = 0; gz < kGridSize; ++gz) {
        for (int gx = 0; gx < kGridSize; ++gx) {
            // 构造一个 5x5 BiomeArea 采样窗口
            BiomeArea area{};

            // 反汇编里对每个格调用 +0xc 槽位的虚函数（采样）
            // 然后逐点计算噪声值，写入 noiseBuffer。
            //
            // 精确公式涉及：
            //   - 4 个 float 的分组插值（unpcklps/subps 那一段）
            //   - 一组 var_XX 常量作为权重
            //   - 最终 divss 得到 [0,1] 的归一化值
            //
            // 因公式依赖具体 biome 的噪声参数，这里只保留结构。
            (void)area;
        }
    }

    // 用 noiseBuffer 填 BlockVolume
    if (isClient) {
        // 客户端变体：走 SSE 优化分支，公式相同
    }

    // 反汇编最后调用 0x35ab390 生成 2D noise，然后循环填充。
    (void)volume;
    (void)pos;
}

// ---------------------------------------------------------------------------
// _makeLayers
//
// 反汇编 block 2 里做的是：把一批 layer 对象（std::shared_ptr<Layer>）
// 塞进一个 vector，每层用不同的 seed 派生。
// ---------------------------------------------------------------------------
void OverworldGenerator::_makeLayers(const LevelData& levelData,
                                     const BiomeRegistry& biomeRegistry)
{
    (void)levelData;
    (void)biomeRegistry;

    // 反汇编里循环里做的事（每层一次）：
    //   shared_ptr<Layer> layer = make_shared<Layer>(parent, seed);
    //   m_layers.push_back(layer);
    //
    // 层的数量由常量 0x20 / 0x2C / 0x1C 等 size 决定，对应不同的
    // Layer 子类（IslandLayer、FuzzyZoomLayer、AddIslandLayer …）。
    //
    // TODO: 按真实 layer 链填充。
}

// ---------------------------------------------------------------------------
// buildSurfaces
//
// 反汇编 block 8：对每个 16x16 的 (x,z)，用噪声决定顶层/次表层方块，
// 然后调用 BlockVolume::setBlock。
// ---------------------------------------------------------------------------
void OverworldGenerator::buildSurfaces(ThreadData& threadData,
                                       BlockVolume& volume,
                                       LevelChunk& chunk,
                                       const ChunkPos& pos)
{
    (void)threadData;

    // 反汇编里第一件事：构造一个 default biome 指针 + 一个 5x5 采样窗口。
    // 然后遍历 16x16：
    //   - 采样 biome
    //   - 采样高度噪声
    //   - 写入 BlockVolume 对应位置
    //
    // 结构如下：
    //
    // for (int z = 0; z < 16; ++z) {
    //     for (int x = 0; x < 16; ++x) {
    //         const Biome* b = chunk.getBiome(x, z);
    //         float height = sampleHeight(x, z);
    //         int blockY = ...;
    //         volume.setBlock(x, blockY, z, surfaceBlockFor(*b));
    //     }
    // }
    //
    // TODO: 补上具体的方块选择逻辑

    (void)volume;
    (void)chunk;
    (void)pos;
}

// ---------------------------------------------------------------------------
// getHeights
//
// 反汇编 block 7：把 BiomeArea 里每个 5x5 格子的高度噪声展开成 425 长度
// 的 float 数组（5 * 5 * 17 = 425）。
// ---------------------------------------------------------------------------
void OverworldGenerator::getHeights(std::array<float, 425>& out,
                                    const BiomeArea& area,
                                    int x, int y, int z)
{
    // 反汇编里对 425 个值逐个做：
    //   xmm0 = noiseValue;
    //   xmm0 = max(xmm0, -1.0f);
    //   xmm0 = xmm0 * 3.0f + 2.0f;
    //   ...
    // 这部分公式已在 block 7 里被展开得很细，此处按语义重构：

    (void)area;
    (void)x; (void)y; (void)z;

    for (size_t i = 0; i < out.size(); ++i) {
        float v = 0.0f;   // TODO: 从 area 采样

        // 反汇编里的 clamp 与重映射：
        //   if (v < -1.0f) v = -1.0f;
        //   v = v * 3.0f + 2.0f;
        //   if (v > 1.0f) v = 1.0f;
        //   v = v * 0.5f + 0.5f;  // → [0,1]
        v = (v < -1.0f) ? -1.0f : v;
        v = v * 3.0f + 2.0f;
        v = (v > 1.0f) ? 1.0f : v;
        v = v * 0.5f + 0.5f;

        out[i] = v;
    }
}

// ---------------------------------------------------------------------------
// _prepareStructureBlueprints
// ---------------------------------------------------------------------------
void OverworldGenerator::_prepareStructureBlueprints(const ChunkPos& pos,
                                                     BiomeSource& biomeSource)
{
    // 反汇编 block 9：对一批 structure registry 逐一调用
    //   registry.prepareBlueprints(pos, biomeSource);
    // 这些 registry 挂在 +0xd8e0 / +0xd95c / +0xda00 / +0xda7c / +0xdaf0 /
    // +0xdb78 / +0xdbfc / +0xdc7c / +0xdcfc / +0xdd74 / +0xde10。
    //
    // 每个 registry 的 prepareBlueprints 是多态调用。

    auto callIfPresent = [&](void* reg) {
        if (!reg) return;
        // reg->vtable[?].prepareBlueprints(pos, biomeSource);
        (void)reg;
    };

    callIfPresent(m_featureRegistry_0xd8e0);
    callIfPresent(m_structureRegistry_0xd95c);
    callIfPresent(m_structureRegistry_0xda00);
    callIfPresent(m_structureRegistry_0xda7c);
    callIfPresent(m_structureRegistry_0xdaf0);
    callIfPresent(m_structureRegistry_0xdb78);
    callIfPresent(m_structureRegistry_0xdbfc);
    callIfPresent(m_structureRegistry_0xdc7c);
    callIfPresent(m_structureRegistry_0xdcfc);
    callIfPresent(m_structureRegistry_0xdd74);
    callIfPresent(m_fileAccess);

    // 反汇编里在末尾还调用 +0xdc7c 那一项的一个 boolean getter，
    // 然后通过它决定是否再调用一次。
    (void)pos;
    (void)biomeSource;
}

// ---------------------------------------------------------------------------
// garbageCollectBlueprints
// ---------------------------------------------------------------------------
void OverworldGenerator::garbageCollectBlueprints(void* span, void* spanEnd)
{
    // 反汇编 block 10：对上面那批 registry 逐一调用
    //   registry.garbageCollectBlueprints(buffer_span<ChunkPos>{span, spanEnd});
    //
    // 每个 registry 的 gc 是 +0x... 槽位的虚函数。

    auto gc = [&](void* reg) {
        if (!reg) return;
        // reg->vtable[?].garbageCollectBlueprints(span, spanEnd);
    };

    gc(m_featureRegistry_0xd8e0);
    gc(m_structureRegistry_0xd95c);
    gc(m_structureRegistry_0xda00);
    gc(m_structureRegistry_0xda7c);
    gc(m_structureRegistry_0xdaf0);
    gc(m_structureRegistry_0xdb78);
    gc(m_structureRegistry_0xdbfc);
    gc(m_structureRegistry_0xdc7c);
    gc(m_structureRegistry_0xdcfc);
    gc(m_structureRegistry_0xdd74);
    gc(m_fileAccess);
}

// ---------------------------------------------------------------------------
// addHardcodedSpawnAreas
// ---------------------------------------------------------------------------
void OverworldGenerator::addHardcodedSpawnAreas(LevelChunk& chunk)
{
    // 反汇编 block 11：只对 +0xdaf0 / +0xda00 / +0xde10 三个 registry
    // 调用 addHardcodedSpawnAreas(chunk)。

    auto call = [&](void* reg) {
        if (!reg) return;
        // reg->vtable[?].addHardcodedSpawnAreas(chunk);
    };

    call(m_structureRegistry_0xdaf0);
    call(m_structureRegistry_0xda00);
    call(m_fileAccess);
}

// ---------------------------------------------------------------------------
// loadChunk
// ---------------------------------------------------------------------------
void OverworldGenerator::loadChunk(LevelChunk& chunk, bool isClient)
{
    // 反汇编 block 12：对 chunk 里的每个 feature registry 调用
    //   registry.loadChunk(chunk, isClient)
    // 然后对 +0xde00 / +0xde08 / +0xddf0 / +0xddf8 那批也调用。
    //
    // 后面的大段代码是构造一个 RNG state 并用它填 +0x21808 起的一块
    // 结构（这是 chunk 的 feature 数据）。

    (void)chunk;
    (void)isClient;
}

// ---------------------------------------------------------------------------
// postProcess
//
// 反汇编 block 13：对区块做后处理，主要是遍历 ChunkViewSource 里
// 每个 chunk，对每个 feature registry 调 postProcess，然后处理
// biome 边界、结构生成。
// ---------------------------------------------------------------------------
void OverworldGenerator::postProcess(ChunkViewSource& source)
{
    // 反汇编 block 13 里对每个 chunk 做的事情：
    //   1. 拿 chunk 的 +0x48 / +0x50 坐标
    //   2. 构造一个 chunk 局部视图
    //   3. 对 11 个 feature registry 调 postProcess
    //   4. 处理水/雪等边界修正

    (void)source;

    // 逐个 registry 调用 postProcess
    // for (auto* reg : getAllRegistries()) {
    //     reg->postProcess(source);
    // }
}

// ---------------------------------------------------------------------------
// _fixWaterAlongEdges
// ---------------------------------------------------------------------------
void OverworldGenerator::_fixWaterAlongEdges(LevelChunk& chunk,
                                             BlockSource& source,
                                             BlockTickingQueue& tickQueue)
{
    // 反汇编 block 14：沿 chunk 的四条边遍历，对每个邻接方块：
    //   - 若为水/岩浆，调度一个 tick
    //
    // 结构：
    //   for (int i = 0; i < edgeLen; ++i) {
    //       for (auto& pos : {edgeNorth[i], edgeSouth[i], edgeEast[i], edgeWest[i]}) {
    //           if (isLiquid(source.getBlock(pos))) {
    //               tickQueue.add(source, pos, getLiquidBlock(pos), delay);
    //           }
    //       }
    //   }

    (void)chunk; (void)source; (void)tickQueue;
}

// ---------------------------------------------------------------------------
// postProcessMobsAt
// ---------------------------------------------------------------------------
void OverworldGenerator::postProcessMobsAt(BlockSource& source,
                                           int x, int z, Random& rng)
{
    // 反汇编 block 15：对 6 个 spawner registry 调用
    //   registry.postProcessMobsAt(source, x, z, rng);

    auto call = [&](void* reg) {
        if (!reg) return;
        // reg->vtable[?].postProcessMobsAt(source, x, z, rng);
    };

    call(m_featureRegistry_0xd8e0);
    call(m_structureRegistry_0xda00);
    call(m_structureRegistry_0xda7c);
    call(m_structureRegistry_0xdaf0);
    call(m_structureRegistry_0xdbfc);
    call(m_fileAccess);
}

// ---------------------------------------------------------------------------
// debugRender
// ---------------------------------------------------------------------------
void OverworldGenerator::debugRender()
{
    // 反汇编 block 16：对每个 registry 调 debugRender()。
    auto call = [](void* reg) {
        if (!reg) return;
        // reg->vtable[?].debugRender();
    };

    call(m_featureRegistry_0xd8e0);
    call(m_structureRegistry_0xd95c);
    call(m_structureRegistry_0xda00);
    call(m_structureRegistry_0xda7c);
    call(m_structureRegistry_0xdaf0);
    call(m_structureRegistry_0xdb78);
    call(m_structureRegistry_0xdbfc);
    call(m_structureRegistry_0xdc7c);
    call(m_structureRegistry_0xdcfc);
    call(m_structureRegistry_0xdd74);
    call(m_fileAccess);
}

// ---------------------------------------------------------------------------
// gatherStats
// ---------------------------------------------------------------------------
void OverworldGenerator::gatherStats()
{
    // 反汇编 block 17：把 "RandomLevelSource" 这个标签塞进 m_stats 里。
    // 这里只做语义等价。
    //
    // m_stats["generator"] = "RandomLevelSource";
}

// ---------------------------------------------------------------------------
// getFeatureTypeAt
//
// 依次询问 11 个 feature registry 是否在该位置有 feature。
// ---------------------------------------------------------------------------
StructureFeatureType
OverworldGenerator::getFeatureTypeAt(const BlockPos& pos)
{
    // 反汇编 block 18：对每个 registry 调用
    //   if (reg->hasFeatureAt(pos)) return type;
    //
    // type 依次是 4、11、3、5、7、6、8、9、13、10、12。

    auto query = [&](void* reg, StructureFeatureType t) -> bool {
        if (!reg) return false;
        // return reg->vtable[?].hasFeatureAt(pos);
        return false;
    };

    if (query(m_structureRegistry_0xdaf0, StructureFeatureType::Feature_OceanMonument))
        return StructureFeatureType::Feature_OceanMonument;
    if (query(m_structureRegistry_0xdb78, StructureFeatureType::Feature_EndCity))
        return StructureFeatureType::Feature_EndCity;
    if (query(m_structureRegistry_0xda7c, StructureFeatureType::Feature_Mineshaft))
        return StructureFeatureType::Feature_Mineshaft;
    if (query(m_structureRegistry_0xd95c, StructureFeatureType::Feature_Stronghold))
        return StructureFeatureType::Feature_Stronghold;
    if (query(m_featureRegistry_0xd8e0, StructureFeatureType::Feature_Ravine))
        return StructureFeatureType::Feature_Ravine;
    if (query(m_structureRegistry_0xda00, StructureFeatureType::Feature_Village))
        return StructureFeatureType::Feature_Village;
    if (query(m_structureRegistry_0xdbfc, StructureFeatureType::Feature_Temple))
        return StructureFeatureType::Feature_Temple;
    if (query(m_structureRegistry_0xdc7c, StructureFeatureType::Feature_WitchHut))
        return StructureFeatureType::Feature_WitchHut;
    if (query(m_structureRegistry_0xdcfc, StructureFeatureType::Feature_Unknown13))
        return StructureFeatureType::Feature_Unknown13;
    if (query(m_structureRegistry_0xdd74, StructureFeatureType::Feature_NetherFortress))
        return StructureFeatureType::Feature_NetherFortress;
    if (query(m_fileAccess, StructureFeatureType::Feature_BuriedTreasure))
        return StructureFeatureType::Feature_BuriedTreasure;

    return StructureFeatureType::None;
}

// ---------------------------------------------------------------------------
// findNearestFeature
// ---------------------------------------------------------------------------
bool OverworldGenerator::findNearestFeature(StructureFeatureType type,
                                            const BlockPos& source,
                                            BlockPos& out)
{
    // 反汇编 block 19：用 switch(type) 分派到具体的 registry。
    //
    // 每个分支都是：
    //   return registry->findNearestFeature(source, out);
    //
    // type 范围 0x03..0x0d，超出则返回 false。

    switch (type) {
    case StructureFeatureType::Feature_Mineshaft:
        // return m_structureRegistry_0xda7c->findNearestFeature(source, out);
        return false;

    case StructureFeatureType::Feature_OceanMonument:
        // return m_structureRegistry_0xdaf0->findNearestFeature(source, out);
        return false;

    case StructureFeatureType::Feature_Stronghold:
        // return m_structureRegistry_0xd95c->findNearestFeature(source, out);
        return false;

    case StructureFeatureType::Feature_Village:
        // return m_structureRegistry_0xda00->findNearestFeature(source, out);
        return false;

    case StructureFeatureType::Feature_Ravine:
        // return m_featureRegistry_0xd8e0->findNearestFeature(source, out);
        return false;

    case StructureFeatureType::Feature_Temple:
        // return m_structureRegistry_0xdbfc->findNearestFeature(source, out);
        return false;

    case StructureFeatureType::Feature_WitchHut:
        // return m_structureRegistry_0xdc7c->findNearestFeature(source, out);
        return false;

    case StructureFeatureType::Feature_NetherFortress:
        // return m_structureRegistry_0xdd74->findNearestFeature(source, out);
        return false;

    case StructureFeatureType::Feature_EndCity:
        // return m_structureRegistry_0xdb78->findNearestFeature(source, out);
        return false;

    case StructureFeatureType::Feature_BuriedTreasure:
        // return m_fileAccess->findNearestFeature(source, out);
        return false;

    case StructureFeatureType::Feature_Unknown13:
        // return m_structureRegistry_0xdcfc->findNearestFeature(source, out);
        return false;

    default:
        return false;
    }
}

// ---------------------------------------------------------------------------
// getBiomeArea
// ---------------------------------------------------------------------------
std::unique_ptr<BiomeArea>
OverworldGenerator::getBiomeArea(const BoundingBox& box, unsigned int area) const
{
    // 反汇编 block 20：委托给成员 +0xde8c 处的 function 对象，它
    // 内部保存了 biome source 的引用。
    //
    // auto& fn = m_callback_0xde8c;
    // return fn(box, area);

    (void)box; (void)area;
    return nullptr;
}

// ---------------------------------------------------------------------------
// findSpawnPosition
// ---------------------------------------------------------------------------
BlockPos OverworldGenerator::findSpawnPosition() const
{
    // 反汇编 block 21：调用 +0x1c 处的虚函数（多重继承的第二基类）。
    //
    // BlockPos result;
    // this->vtableSecondary[?].findSpawnPosition(result);
    // return result;

    BlockPos result;
    // TODO: 具体逻辑在第二基类
    result.x = 0;
    result.y = 32767;   // 0x7fff，反汇编里的默认值
    result.z = 0;
    return result;
}

// ===========================================================================
// OverridableBiomeSourceHelper
// ===========================================================================
OverworldGenerator::OverridableBiomeSourceHelper::
OverridableBiomeSourceHelper(const OverworldGenerator& owner)
{
    // 反汇编 block 22：
    //   1. 从 owner 的 +0xde9c 处拿一个 shared_ptr 风格的
    //      {ctrlBlock, ctrlBlock2, object} 三元组
    //   2. 拷贝到自身

    // 反汇编里对 owner+0xde9c 的访问：
    //   m_ctrlBlock  = *(void**)(owner + 0xde9c);
    //   m_object     = *(void**)(owner + 0xdea0);
    //   m_ctrlBlock2 = *(void**)(owner + 0xdea4);

    // 拷贝时对引用计数做原子递增
    if (owner.m_callbackCapture) {
        m_ctrlBlock  = owner.m_callbackCapture;
        m_ctrlBlock2 = owner.m_callbackCapture;
        // std::__ndk1::__shared_weak_count::__add_shared(m_ctrlBlock);
    }
    m_object = const_cast<OverworldGenerator*>(&owner);
}