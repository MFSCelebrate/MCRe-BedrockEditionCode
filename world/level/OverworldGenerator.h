#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// 前向声明
// ---------------------------------------------------------------------------
class Dimension;
class Biome;
class BlockVolume;
class ChunkPos;
class BiomeSource;
class BlockSource;
class BlockTickingQueue;
class LevelChunk;
class LevelData;
class BiomeRegistry;
class BiomeArea;
class ChunkViewSource;
class Random;
class IFileAccess;
class AppPlatform;

// 反汇编里 OverworldGenerator 持有一堆 JsonUtil schema 对象，都是 opaque 的
namespace JsonUtil {
    struct JsonSchemaObjectNode {};
    struct JsonSchemaArrayNode  {};
    struct JsonSchemaEnumNode   {};
}

// ---------------------------------------------------------------------------
// StructureFeatureType
//
// 反汇编中 getFeatureTypeAt / findNearestFeature 用到，取值范围 0x03..0x0d。
// ---------------------------------------------------------------------------
enum class StructureFeatureType : uint8_t {
    None                    = 0x00,
    Feature_Mineshaft       = 0x03,   // getFeatureTypeAt +0xda7c → 3
    Feature_OceanMonument   = 0x04,   // +0xdaf0 → 4
    Feature_Stronghold      = 0x05,   // +0xd95c → 5
    Feature_Village         = 0x06,   // +0xda00 → 6
    Feature_Ravine          = 0x07,   // +0xd8e0 → 7
    Feature_Temple          = 0x08,   // +0xdbfc → 8
    Feature_WitchHut        = 0x09,   // +0xdc7c → 9
    Feature_NetherFortress  = 0x0a,   // +0xdd74 → 10
    Feature_EndCity         = 0x0b,   // +0xdb78 → 11
    Feature_BuriedTreasure  = 0x0c,   // +0xde10 → 12
    Feature_Unknown13       = 0x0d,   // +0xdcfc → 13
};

// ---------------------------------------------------------------------------
// OverworldGenerator
//
// 布局严格按反汇编 offset 排布。这个类很大（约 0xDEC0 字节），属于
// 多重继承：vtable 在 +0x00 和 +0x1c 两处，构造/析构里都出现两次。
// ---------------------------------------------------------------------------
class OverworldGenerator /* : public WorldGenerator */ {
public:
    // ========================================================================
    // ThreadData
    //
    // buildSurfaces() 里用到的线程本地状态。反汇编没展开它，这里给空壳。
    // ========================================================================
    struct ThreadData {
        // 反汇编对 ThreadData& 只调用其方法（0x34984c0、0x34db3d0 等），
        // 没有访问内部字段，所以这里留空。
        char _opaque[1];
    };

    // ========================================================================
    // OverridableBiomeSourceHelper
    //
    // 见 block 22。构造时从 OverworldGenerator 的 +0xde9c 处
    // 生成一个带引用计数的包装对象。
    // ========================================================================
    class OverridableBiomeSourceHelper {
    public:
        explicit OverridableBiomeSourceHelper(const OverworldGenerator& owner);
        ~OverridableBiomeSourceHelper() = default;

    private:
        // 反汇编里这个 helper 持有一个 shared_ptr 风格的 3 字段结构：
        //   [0] = 引用计数块的指针
        //   [4] = 同上（另一个引用）
        //   [8] = 原始对象指针
        void* m_ctrlBlock = nullptr;
        void* m_ctrlBlock2 = nullptr;
        void* m_object    = nullptr;
    };

    // ========================================================================
    // 生命周期
    // ========================================================================
    OverworldGenerator(Dimension& dimension,
                       unsigned int seed,
                       bool         isClient,
                       Biome const* defaultBiome);

    virtual ~OverworldGenerator();

    // ========================================================================
    // 生成管线
    // ========================================================================

    // _registerFeatures 类：反汇编里在构造函数中一次性注册所有 feature/schema
    void _registerFeatures();

    // _makeLayers(LevelData const&, BiomeRegistry const&)
    void _makeLayers(const LevelData& levelData,
                     const BiomeRegistry& biomeRegistry);

    // _prepareHeights(BlockVolume&, ChunkPos const&, BiomeSource const&, bool)
    void _prepareHeights(BlockVolume& volume,
                         const ChunkPos& pos,
                         const BiomeSource& biomeSource,
                         bool isClient);

    // 公开入口：prepareHeights(BlockVolume&, ChunkPos const&, bool)
    void prepareHeights(BlockVolume& volume,
                        const ChunkPos& pos,
                        bool isClient);

    // buildSurfaces(ThreadData&, BlockVolume&, LevelChunk&, ChunkPos const&)
    void buildSurfaces(ThreadData& threadData,
                       BlockVolume& volume,
                       LevelChunk& chunk,
                       const ChunkPos& pos);

    // getHeights(array<float,425>&, BiomeArea const&, int, int, int)
    void getHeights(std::array<float, 425>& out,
                    const BiomeArea& area,
                    int x, int y, int z);

    // _prepareStructureBlueprints(ChunkPos const&, BiomeSource&)
    void _prepareStructureBlueprints(const ChunkPos& pos,
                                     BiomeSource& biomeSource);

    // garbageCollectBlueprints(buffer_span<ChunkPos>)
    void garbageCollectBlueprints(/* buffer_span<ChunkPos> */ void* span,
                                  void* spanEnd);

    // addHardcodedSpawnAreas(LevelChunk&)
    void addHardcodedSpawnAreas(LevelChunk& chunk);

    // loadChunk(LevelChunk&, bool)
    void loadChunk(LevelChunk& chunk, bool isClient);

    // postProcess(ChunkViewSource&)
    void postProcess(ChunkViewSource& source);

    // _fixWaterAlongEdges(LevelChunk&, BlockSource&, BlockTickingQueue&)
    void _fixWaterAlongEdges(LevelChunk& chunk,
                             BlockSource& source,
                             BlockTickingQueue& tickQueue);

    // postProcessMobsAt(BlockSource&, int, int, Random&)
    void postProcessMobsAt(BlockSource& source,
                           int x, int z, Random& rng);

    // ========================================================================
    // 调试 / 统计
    // ========================================================================
    void debugRender();
    void gatherStats();

    // ========================================================================
    // 查询
    // ========================================================================
    StructureFeatureType getFeatureTypeAt(const class BlockPos& pos);

    bool findNearestFeature(StructureFeatureType type,
                            const class BlockPos& source,
                            class BlockPos&       out);

    std::unique_ptr<class BiomeArea>
    getBiomeArea(const class BoundingBox& box, unsigned int area) const;

    class BlockPos findSpawnPosition() const;

private:
    // ========================================================================
    // 内部：析构辅助
    // ========================================================================
    void _destroyMembers();

    // ========================================================================
    // 成员（严格按反汇编 offset 排列）
    //
    // 精确 layout 见每个字段的注释。为便于后续改版，这里用 #pragma pack 保持
    // 显式 padding，避免编译器改变偏移。
    // ========================================================================
#pragma pack(push, 4)

    // ---- 基类 vtable + 多重继承第二 vtable ----
    void* _vtablePrimary       = nullptr;   // +0x00
    char  _pad_004_[0x18]      = {};        // +0x04 .. +0x1b (基类其他字段)
    void* _vtableSecondary     = nullptr;   // +0x1c

    // ---- 基础参数 ----
    bool  m_isClient           = false;     // +0x24
    char  _pad_025_[3]         = {};
    Random* m_random           = nullptr;   // +0x28  (0x10 字节对象，见构造 block 1)

    // ---- 各种缓存的 schema / feature 注册表 ----
    // 这些在构造 block 1 / 析构 block 3 里被逐个初始化/销毁，
    // 类型是 std::shared_ptr<JsonUtil::...> 或类似结构。
    // 每个字段偏移直接标出。
    void* m_schema_0xd828      = nullptr;   // +0xd828
    void* m_schema_0xd82c      = nullptr;   // +0xd82c
    void* m_schema_0xd830      = nullptr;   // +0xd830
    void* m_schema_0xd834      = nullptr;   // +0xd834
    void* m_schema_0xd838      = nullptr;   // +0xd838
    void* m_schema_0xd83c      = nullptr;   // +0xd83c
    void* m_schema_0xd840      = nullptr;   // +0xd840
    void* m_schema_0xd844      = nullptr;   // +0xd844

    // 一组 float 常量（噪声参数），构造 block 1 里全部被赋值为
    // m_floatA / 具体除数，共 24 个 float，范围 +0xd848..+0xd8a8。
    float m_noiseParams[24]    = {};        // +0xd848 .. +0xd8a8

    // ---- 主 schema 区域 ----
    void* m_schema_0xd8b0      = nullptr;   // +0xd8b0
    char  _pad_d8b4_[0x0c]     = {};
    void* m_schema_0xd8c0      = nullptr;   // +0xd8c0
    void* m_schema_0xd8c8      = nullptr;   // +0xd8c8
    void* m_schema_0xd8cc      = nullptr;   // +0xd8cc
    void* m_schema_0xd8d0      = nullptr;   // +0xd8d0
    void* m_schema_0xd8d4      = nullptr;   // +0xd8d4  (pthread_key 相关)
    bool  m_schemaInitialized  = false;     // +0xd8d8

    // feature 注册表 / 各类 schema 指针（详见析构 block 3，逐个被 reset）
    void* m_featureRegistry_0xd8e0 = nullptr;   // +0xd8e0
    char  _pad_d8e4_[0x78]         = {};
    void* m_structureRegistry_0xd95c = nullptr; // +0xd95c
    char  _pad_d960_[0x84]         = {};
    std::mutex m_mutex_0xd9e4{};                // +0xd9e4
    void* m_structureRegistry_0xda00 = nullptr; // +0xda00
    char  _pad_da04_[0x78]         = {};
    void* m_structureRegistry_0xda7c = nullptr; // +0xda7c
    char  _pad_da80_[0x64]         = {};
    void* m_structureRegistry_0xdae4 = nullptr; // +0xdae4
    void* m_structureRegistry_0xdaf0 = nullptr; // +0xdaf0
    char  _pad_daf4_[0x84]         = {};
    void* m_structureRegistry_0xdb78 = nullptr; // +0xdb78
    char  _pad_db7c_[0x80]         = {};
    void* m_structureRegistry_0xdbfc = nullptr; // +0xdbfc
    char  _pad_dc00_[0x7c]         = {};
    void* m_structureRegistry_0xdc7c = nullptr; // +0xdc7c
    char  _pad_dc80_[0x7c]         = {};
    void* m_structureRegistry_0xdcfc = nullptr; // +0xdcfc
    char  _pad_dd00_[0x74]         = {};
    void* m_structureRegistry_0xdd74 = nullptr; // +0xdd74
    char  _pad_dd78_[0x78]         = {};

    // ---- 两个 feature 列表 ----
    void* m_featureList_0xddf0 = nullptr;       // +0xddf0
    void* m_featureList_0xddf8 = nullptr;       // +0xddf8
    void* m_featureList_0xde00 = nullptr;       // +0xde00
    void* m_featureList_0xde08 = nullptr;       // +0xde08

    // ---- IFileAccess 接口 ----
    IFileAccess* m_fileAccess  = nullptr;       // +0xde10

    char  _pad_de14_[0x78]     = {};

    // ---- 一个 std::function（析构 block 3 里最后被清理） ----
    std::function<void()> m_callback_0xde8c;    // +0xde8c .. +0xdea4
    void* m_callbackCapture    = nullptr;       // +0xdea4

#pragma pack(pop)

    // 反汇编里 +0xde8c 那个 function 有 24 字节（libc++ 的 std::function 布局），
    // 上面 std::function 会占掉对应空间，m_callbackCapture 只是占位，不实际使用。
};

// 静态断言：OverworldGenerator 至少到 0xDEB0 字节，具体大小以实测为准。
static_assert(sizeof(OverworldGenerator) >= 0xDEB0,
              "OverworldGenerator layout mismatch");