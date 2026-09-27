// ============================================================================
// FeatureToggles.hpp
//
// 从 Minecraft Bedrock 逆向还原的重构版本。
//
// 说明：
//   - 原版是 x86 32 位 ARM/Android 反汇编，很多成员布局 (offset) 依赖
//     libc++ (std::__ndk1) 的具体实现；
//   - 本文件保留原有 offset 关系，用成员声明顺序保证布局大体一致；
//   - 字符串常量中只有 "Enable Packet Profiling" / "enable_packet_profile"
//     能从反汇编中直接读出，其余用占位符表示；
//   - 部分函数用 inline 实现，方便直接阅读。
// ============================================================================

#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <utility>

// ---------------------------------------------------------------------------
// 前向声明
// ---------------------------------------------------------------------------
class AppPlatform;
class Option;
class FeatureToggles;

// ============================================================================
// FeatureOptionID
//
// 从 _registerFeatures() 反汇编里收集到的所有注册 ID。
// 值即为原 4 字节整型值。
// ============================================================================
enum class FeatureOptionID : int32_t {
    None                = 0x00,

    // 已知：字符串 "Enable Packet Profiling" / "enable_packet_profile"
    PacketProfiling     = 0x01,

    Feature0x02         = 0x02,
    Feature0x03         = 0x03,
    Feature0x04         = 0x04,
    Feature0x05         = 0x05,
    Feature0x06         = 0x06,
    Feature0x07         = 0x07,
    Feature0x08         = 0x08,

    Feature0x0A         = 0x0a,
    Feature0x0B         = 0x0b,
    Feature0x0C         = 0x0c,
    Feature0x0D         = 0x0d,
    Feature0x0E         = 0x0e,
    Feature0x0F         = 0x0f,
    Feature0x10         = 0x10,
    Feature0x13         = 0x13,
    Feature0x14         = 0x14,
    Feature0x15         = 0x15,
    Feature0x16         = 0x16,
    Feature0x18         = 0x18,
    Feature0x19         = 0x19,
    Feature0x1A         = 0x1a,
    Feature0x1B         = 0x1b,
    Feature0x1D         = 0x1d,
    Feature0x1E         = 0x1e,
    Feature0x20         = 0x20,
    Feature0x21         = 0x21,
    Feature0x22         = 0x22,
    Feature0x23         = 0x23,
    Feature0x24         = 0x24,
    Feature0x26         = 0x26,
};

// ============================================================================
// Option
//
// 对应反汇编里每个 FeatureToggle 持有的那个对象，大小约 0xc0 字节。
//
// 关键 offset：
//   +0x00  vtable 指针
//   +0xa0  m_enabled          (bool)
//   +0xa1  m_defaultEnabled   (bool)
//   +0xb8  m_extraPtr
//
// vtable 里出现的相关槽位（Android libc++ 布局）：
//   +0x08  某个返回指针的虚函数
//   +0x0c  某个返回指针的虚函数
//   +0x10  析构
//   +0x14  删除析构
// ============================================================================
class Option {
public:
    virtual ~Option() = default;

    virtual void initialize(AppPlatform& /*appPlatform*/) {}
    virtual void setupDependencies(FeatureToggles& /*toggles*/) {}

    bool isEnabled() const noexcept { return m_enabled; }
    void setEnabled(bool v) noexcept { m_enabled = v; }

    bool isDefaultEnabled() const noexcept { return m_defaultEnabled; }

protected:
    // ---- +0xa0 ~ +0xb8 ----
    bool  m_enabled        = false;
    bool  m_defaultEnabled = false;
    char  _pad0a2_[0x16]   = {};

    void* m_extraPtr       = nullptr; // +0xb8
};

// ============================================================================
// FeatureToggles
// ============================================================================
class FeatureToggles {
public:
    // -- 回调类型 --
    using SetupCallback   = std::function<void(Option&)>;
    using EnabledCallback = std::function<void(bool&)>;

    // ========================================================================
    // FeatureToggle
    //
    // 每个元素大小 = 0x40 字节，对应 count() 里的 (end - begin) >> 6。
    //
    // offset 布局：
    //   +0x00  id            (FeatureOptionID)
    //   +0x04  dependency    (FeatureOptionID)
    //   +0x08  option        (std::unique_ptr<Option>)
    //   +0x10  setupCallback (std::function<void(Option&)>)
    //   +0x28  enabledCb     (std::function<void(bool&)>)
    //   = 0x40
    // ========================================================================
    struct FeatureToggle {
        FeatureOptionID               id         = FeatureOptionID::None;
        FeatureOptionID               dependency = FeatureOptionID::None;
        std::unique_ptr<Option>       option;
        SetupCallback                 setupCallback;
        EnabledCallback               enabledCallback;
    };

    // ========================================================================
    // 生命周期
    // ========================================================================
    FeatureToggles();
    ~FeatureToggles();

    // 原版反汇编中这几个函数体几乎为空
    void _load() {}
    void _save() {}

    // ========================================================================
    // 核心流程
    // ========================================================================
    void _registerFeatures();
    void _initialize(AppPlatform& appPlatform);
    void _setupDependencies();

    // ========================================================================
    // 查询
    // ========================================================================
    std::size_t count() const noexcept {
        return m_features.size();
    }

    Option* get(FeatureOptionID id) {
        for (auto& f : m_features) {
            if (f.id == id) {
                return f.option.get();
            }
        }
        return nullptr;
    }

    const Option* get(FeatureOptionID id) const {
        for (const auto& f : m_features) {
            if (f.id == id) {
                return f.option.get();
            }
        }
        return nullptr;
    }

    bool isEnabled(FeatureOptionID id) const {
        const Option* opt = get(id);
        return opt && opt->isEnabled();
    }

    // ========================================================================
    // 注册单个 feature（对应 0x33ac0e0）
    //
    // 反汇编 mangled name：
    //   _ZN14FeatureToggles16_registerFeatureE15FeatureOptionID
    //       RKNSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE
    //       S9_
    //       b
    //       S0_
    //       NS1_8functionIFvR6OptionEEE
    //       NSA_IFvRbEEE
    // ========================================================================
    void _registerFeature(
        FeatureOptionID              id,
        const std::string&           displayName,
        const std::string&           configKey,
        bool                         defaultValue,
        FeatureOptionID              dependency,
        SetupCallback                setupCallback,
        EnabledCallback              enabledCallback
    );

    // ========================================================================
    // 回调获取
    //
    // 反汇编里这两个函数返回一个绑定了 (this, id) 的 std::function，
    // 内部再转发到 FeatureToggle 里保存的 setupCallback / enabledCallback。
    // ========================================================================
    SetupCallback
    _getEnabledSetupCallback(FeatureOptionID id);

    EnabledCallback
    _getEnabledLockedCallback(FeatureOptionID id);

private:
    // ------------------------------------------------------------------------
    // 成员（顺序严格按反汇编 offset 排列）
    // ------------------------------------------------------------------------
    // 说明：+0x00 ~ +0x07 通常被编译器用于对齐/保留
    std::uint32_t _reserved0 = 0;
    std::uint32_t _reserved1 = 0;

    // +0x08  std::vector<FeatureToggle>  (begin / end / cap)
    std::vector<FeatureToggle> m_features;

    // +0x14  std::vector<std::shared_ptr<void>>  依赖容器
    std::vector<std::shared_ptr<void>> m_dependencies;

    // +0x20  内部存储路径字符串（来自 AppPlatform::getInternalStoragePath()）
    // 在 _initialize() 里赋值，析构时会释放。
    std::string m_internalStoragePath;
};

// ============================================================================
// ============================================================================
//                              实现部分
// ============================================================================
// ============================================================================

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------
inline FeatureToggles::FeatureToggles()  = default;
inline FeatureToggles::~FeatureToggles() = default;

// ---------------------------------------------------------------------------
// _registerFeature
// ---------------------------------------------------------------------------
inline void FeatureToggles::_registerFeature(
    FeatureOptionID    id,
    const std::string& displayName,
    const std::string& configKey,
    bool               defaultValue,
    FeatureOptionID    dependency,
    SetupCallback      setupCallback,
    EnabledCallback    enabledCallback
) {
    // 反汇编里先 operator new(0xc0) 构造 Option
    auto option = std::make_unique<Option>();

    // 默认值与显示名/配置键会在 Option 构造流程里使用
    option->m_defaultEnabled = defaultValue;
    option->m_enabled        = defaultValue;
    (void)displayName;   // 由 Option 子类或具体构造流程消费
    (void)configKey;

    FeatureToggle toggle;
    toggle.id              = id;
    toggle.dependency      = dependency;
    toggle.option          = std::move(option);
    toggle.setupCallback   = std::move(setupCallback);
    toggle.enabledCallback = std::move(enabledCallback);

    // 反汇编里在 push_back 前会有一次容量检查
    m_features.push_back(std::move(toggle));
}

// ---------------------------------------------------------------------------
// _registerFeatures
//
// 原函数由 31 个几乎相同的代码块组成。这里改成表驱动。
//
// 表格字段：
//   id, displayName, configKey, defaultValue, dependency
//
// 说明：
//   - 字符串只能从偏移上推断顺序，无法还原真实文本；
//     下面除第一条外，其余使用占位符，请对照真实字符串表替换。
//   - defaultValue 来自反汇编里 var_10h 的值。
// ---------------------------------------------------------------------------
inline void FeatureToggles::_registerFeatures() {
    struct FeatureDef {
        FeatureOptionID id;
        const char*     displayName;
        const char*     configKey;
        bool            defaultValue;
        FeatureOptionID dependency;
    };

    static constexpr FeatureDef kFeatureDefs[] = {
        // ---- 已知 ----
        { FeatureOptionID::PacketProfiling, "Enable Packet Profiling", "enable_packet_profile", true,  FeatureOptionID::None },

        // ---- 从反汇编 var_4h / var_10h 推出的注册顺序 ----
        { FeatureOptionID::Feature0x0A, "<unknown>", "<unknown>", true,  FeatureOptionID::None },
        { FeatureOptionID::Feature0x0B, "<unknown>", "<unknown>", true,  FeatureOptionID::None },
        { FeatureOptionID::Feature0x04, "<unknown>", "<unknown>", true,  FeatureOptionID::None },
        { FeatureOptionID::Feature0x02, "<unknown>", "<unknown>", true,  FeatureOptionID::None },
        { FeatureOptionID::Feature0x03, "<unknown>", "<unknown>", true,  FeatureOptionID::None },
        { FeatureOptionID::Feature0x05, "<unknown>", "<unknown>", false, FeatureOptionID::None },

        { FeatureOptionID::Feature0x20, "<unknown>", "<unknown>", true,  FeatureOptionID::None },
        { FeatureOptionID::Feature0x23, "<unknown>", "<unknown>", true,  FeatureOptionID::None },
        { FeatureOptionID::Feature0x26, "<unknown>", "<unknown>", false, FeatureOptionID::None },
        { FeatureOptionID::Feature0x1D, "<unknown>", "<unknown>", false, FeatureOptionID::None },
        { FeatureOptionID::Feature0x1E, "<unknown>", "<unknown>", true,  FeatureOptionID::None },
        { FeatureOptionID::Feature0x0D, "<unknown>", "<unknown>", true,  FeatureOptionID::None },
        { FeatureOptionID::Feature0x0C, "<unknown>", "<unknown>", false, FeatureOptionID::None },

        // ---- 后续块，很多在 var_14h = 6 / 0 上做条件注册 ----
        { FeatureOptionID::Feature0x06, "<unknown>", "<unknown>", false, FeatureOptionID::None },
        { FeatureOptionID::Feature0x08, "<unknown>", "<unknown>", false, FeatureOptionID::None },
        { FeatureOptionID::Feature0x07, "<unknown>", "<unknown>", false, FeatureOptionID::None },
        { FeatureOptionID::Feature0x0E, "<unknown>", "<unknown>", false, FeatureOptionID::None },
        { FeatureOptionID::Feature0x1A, "<unknown>", "<unknown>", false, FeatureOptionID::None },
        { FeatureOptionID::Feature0x1B, "<unknown>", "<unknown>", true,  FeatureOptionID::None },
        { FeatureOptionID::Feature0x0F, "<unknown>", "<unknown>", true,  FeatureOptionID::None },
        { FeatureOptionID::Feature0x10, "<unknown>", "<unknown>", false, FeatureOptionID::None },
        { FeatureOptionID::Feature0x13, "<unknown>", "<unknown>", false, FeatureOptionID::None },
        { FeatureOptionID::Feature0x14, "<unknown>", "<unknown>", true,  FeatureOptionID::None },
        { FeatureOptionID::Feature0x15, "<unknown>", "<unknown>", false, FeatureOptionID::None },
        { FeatureOptionID::Feature0x16, "<unknown>", "<unknown>", false, FeatureOptionID::None },
        { FeatureOptionID::Feature0x18, "<unknown>", "<unknown>", false, FeatureOptionID::None },
        { FeatureOptionID::Feature0x19, "<unknown>", "<unknown>", true,  FeatureOptionID::None },
        { FeatureOptionID::Feature0x21, "<unknown>", "<unknown>", true,  FeatureOptionID::None },
        { FeatureOptionID::Feature0x22, "<unknown>", "<unknown>", false, FeatureOptionID::None },
        { FeatureOptionID::Feature0x24, "<unknown>", "<unknown>", false, FeatureOptionID::None },
    };

    m_features.reserve(std::size(kFeatureDefs));

    for (const auto& def : kFeatureDefs) {
        _registerFeature(
            def.id,
            def.displayName,
            def.configKey,
            def.defaultValue,
            def.dependency,
            /*setupCallback   */ {},
            /*enabledCallback */ {}
        );
    }
}

// ---------------------------------------------------------------------------
// _initialize
//
// 反汇编里做了三件事：
//   1. 从 AppPlatform 拿到 internal storage path，保存到 m_internalStoragePath
//   2. 遍历所有 feature，调用 option 的 initialize 流程
//   3. 处理一些 AppPlatform 相关的容器（+0x08/+0x0c 那段循环）
// ---------------------------------------------------------------------------
inline void FeatureToggles::_initialize(AppPlatform& appPlatform) {
    // 反汇编里通过 vtable +0x168 取得 internal storage path
    // 这里按语义简化：
    // m_internalStoragePath = appPlatform.getInternalStoragePath();

    for (auto& f : m_features) {
        if (f.option) {
            f.option->initialize(appPlatform);
        }
    }
}

// ---------------------------------------------------------------------------
// _setupDependencies
//
// 反汇编里遍历 m_features，对每个 option 调用 setupDependencies，
// 并且通过 AppPlatform 处理一些 shared_ptr 弱引用释放。
// ---------------------------------------------------------------------------
inline void FeatureToggles::_setupDependencies() {
    for (auto& f : m_features) {
        if (f.option) {
            f.option->setupDependencies(*this);
        }
    }
}

// ---------------------------------------------------------------------------
// _getEnabledSetupCallback
//
// 对应反汇编块 13：
//   返回一个函子，内部保存 (this, id)，调用时转发到注册时的 setupCallback。
// ---------------------------------------------------------------------------
inline FeatureToggles::SetupCallback
FeatureToggles::_getEnabledSetupCallback(FeatureOptionID id) {
    return [this, id](Option& opt) {
        Option* target = get(id);
        if (!target) {
            return;
        }

        for (auto& f : m_features) {
            if (f.id == id && f.setupCallback) {
                f.setupCallback(opt);
                return;
            }
        }
    };
}

// ---------------------------------------------------------------------------
// _getEnabledLockedCallback
//
// 对应反汇编块 14：同上，转发到 enabledCallback。
// ---------------------------------------------------------------------------
inline FeatureToggles::EnabledCallback
FeatureToggles::_getEnabledLockedCallback(FeatureOptionID id) {
    return [this, id](bool& value) {
        Option* target = get(id);
        if (!target) {
            return;
        }

        for (auto& f : m_features) {
            if (f.id == id && f.enabledCallback) {
                f.enabledCallback(value);
                return;
            }
        }
    };
}