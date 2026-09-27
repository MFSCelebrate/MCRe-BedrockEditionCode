#include "Minecraft.hpp"

#include <cstdint>
#include <cstring>
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// ===========================================================================
// 依赖类的极简 stub（真实项目里替换为对应实现）
// ===========================================================================
class IMinecraftApp {};
class GameCallbacks {};
class AllowList {};
class PermissionsFile {};
namespace Core { class FilePathManager {}; }
class IMinecraftEventing {};
class NetworkHandler {};
class PacketSender {};
class Timer {};
class ContentTierManager {};
class ServerMetrics {};
class Level {};
class Player {};
class NetEventCallback {};
class Scheduler {};
class TextFilteringProcessor {};
class ResourceLoader {};
class StructureManager {};
class EntityRegistry {};
class NetworkIdentifier {};
class GameModuleServer {};
class LevelChunk {};

namespace mce {
    struct UUID {};
}

struct ConnectionDefinition {};
struct PackIdVersion {};
struct PackIdVersionHash {
    size_t operator()(const PackIdVersion&) const { return 0; }
};
struct PackIdVersionEq {
    bool operator()(const PackIdVersion&, const PackIdVersion&) const { return false; }
};

enum class GameType : int {
    Survival = 0,
    Creative = 1,
    Adventure = 2,
    Spectator = 3,
};

// ===========================================================================
// Minecraft
//
// 布局（根据反汇编 offset 推断）：
//   +0x00  vtable（主）
//   +0x04  GameCallbacks*
//   +0x08  IMinecraftEventing*
//   +0x0c  ResourceLoader*
//   +0x10  StructureManager*
//   +0x14  m_gameModuleServer（secondary vtable 相关）
//   +0x18  shared_ptr 控制块
//   +0x1c  PermissionsFile*
//   +0x20  Core::FilePathManager*
//   +0x24  shared_ptr<EntityRegistry>
//   +0x28  Timer 实例（0x10 字节对象）
//   +0x34  chrono::duration（64 位）
//   +0x38  ServerMetrics*
//   +0x3c  bool（用于 leaveGame 状态）
//   +0x40  steady_clock::time_point
//   +0x48  steady_clock::time_point（上一帧）
//   +0x50  pair（FileAccess 相关）
//   +0x58  Commands*
//   +0x5c  NetEventCallback / session 唯一指针
//   +0x60  Timer*（模拟时间）
//   +0x64  Timer*（实时时间）
//   +0x68  NetworkHandler*
//   +0x6c  PacketSender*
//   +0x70  IMinecraftApp*
//   +0x74  uint8_t clientSubId
//   +0x78  EntityRegistry / 其他成员
// ===========================================================================
class Minecraft {
public:
    // ---- 生命周期 ----
    Minecraft(IMinecraftApp& app,
              GameCallbacks& callbacks,
              AllowList& allowList,
              PermissionsFile* permissionsFile,
              Core::FilePathManager* filePathManager,
              std::chrono::duration<long long, std::ratio<1, 1>> duration,
              IMinecraftEventing& eventing,
              NetworkHandler& networkHandler,
              PacketSender& packetSender,
              unsigned char clientSubId,
              Timer& simTimer,
              Timer& realTimer,
              const ContentTierManager& contentTierManager,
              ServerMetrics* serverMetrics);

    virtual ~Minecraft();
    static void operator delete(void* p);

    // ---- 会话 / 生命周期 ----
    void resetGameSession();
    void clearThreadCallbacks();
    void updateScreens();
    void initAsDedicatedServer();
    void init();
    GameModuleServer* getGameModuleServer();
    void initCommands();
    bool isInitialized() const;
    void startLeaveGame(bool fade);
    bool isLeaveGameDone() const;
    void clientReset();
    void setGameModeReal(GameType gameType);
    void update();

    // ---- 服务器网络 ----
    NetworkHandler* getServerNetworkHandler();
    void disconnectClient(const NetworkIdentifier& id, const std::string& reason);
    void tickSimtime(int a, int b);
    void tickRealtime(int a, int b);
    Level* getLevel() const;

    // ---- 模拟时间 ----
    void setSimTimePause(bool paused);
    void setSimTimeScale(float scale);
    bool getSimPaused() const;

    // ---- 查询 ----
    bool isModded();
    bool isOnlineClient();
    void getNetworkStatistics() const;

    // ---- 多人游戏 ----
    void hostMultiplayer(
        const std::string& worldName,
        std::unique_ptr<Level> level,
        Player* player,
        const mce::UUID& uuid,
        std::unique_ptr<NetEventCallback> netCallback,
        int a, bool b, bool c,
        const std::vector<std::string>& packs,
        std::string connectionStr,
        const ConnectionDefinition& connDef,
        const std::unordered_map<
            PackIdVersion,
            std::string,
            PackIdVersionHash,
            PackIdVersionEq>& packMap,
        Scheduler& scheduler,
        TextFilteringProcessor* filter);

    unsigned char getClientSubId() const;

    void setupServerCommands(const std::string& a, const std::string& b);
    bool usesNonLocalConnection(const NetworkIdentifier& id);
    NetworkHandler* getNetworkHandler();
    void startClientGame(std::unique_ptr<NetEventCallback> cb);
    void joinWorldInProgress(std::unique_ptr<NetEventCallback> cb);
    void activateAllowList();
    void* getCommands();
    bool hasCommands();
    NetEventCallback* getNetEventCallback();
    NetworkHandler* getServerLocator();
    const NetworkHandler* getServerLocator() const;
    NetworkHandler* getNetworkHandler() const;
    IMinecraftEventing* getEventing() const;
    void onClientCreatedLevel(std::unique_ptr<Level> level);
    Timer* getTimer();
    double getLastTimestep();
    ResourceLoader* getResourceLoader();
    StructureManager* getStructureManager();
    EntityRegistry* getEntityRegistry();

private:
    // =======================================================================
    // 成员（严格按反汇编 offset 排列）
    // =======================================================================
#pragma pack(push, 4)

    void* m_vtablePrimary;           // +0x00
    GameCallbacks* m_callbacks;      // +0x04
    IMinecraftEventing* m_eventing;  // +0x08
    ResourceLoader* m_resourceLoader;// +0x0c
    StructureManager* m_structureManager; // +0x10
    void* m_gameModuleServer;        // +0x14
    void* m_entityRegistryCtrl;      // +0x18
    PermissionsFile* m_permissionsFile;   // +0x1c
    Core::FilePathManager* m_filePathManager; // +0x20
    void* m_entityRegistry;          // +0x24
    Timer m_simTimeObj;              // +0x28 (0x10 字节)
    std::chrono::duration<long long, std::ratio<1, 1>> m_duration; // +0x34
    ServerMetrics* m_serverMetrics;  // +0x38
    bool m_leaveGameFlag;            // +0x3c
    char _pad_3d_[3];
    std::chrono::steady_clock::time_point m_lastFrameTime; // +0x40
    std::chrono::steady_clock::time_point m_prevFrameTime; // +0x48
    void* m_fileAccess;              // +0x50
    void* m_fileAccessEnd;           // +0x54
    void* m_commands;                // +0x58
    void* m_session;                 // +0x5c
    Timer* m_simTimer;               // +0x60
    Timer* m_realTimer;              // +0x64
    NetworkHandler* m_networkHandler;// +0x68
    PacketSender* m_packetSender;    // +0x6c
    IMinecraftApp* m_app;            // +0x70
    unsigned char m_clientSubId;     // +0x74
    char _pad_75_[3];
    EntityRegistry m_entityRegistryObj; // +0x78

#pragma pack(pop)
};

// ===========================================================================
// 构造函数
// ===========================================================================
Minecraft::Minecraft(IMinecraftApp& app,
                     GameCallbacks& callbacks,
                     AllowList& allowList,
                     PermissionsFile* permissionsFile,
                     Core::FilePathManager* filePathManager,
                     std::chrono::duration<long long, std::ratio<1, 1>> duration,
                     IMinecraftEventing& eventing,
                     NetworkHandler& networkHandler,
                     PacketSender& packetSender,
                     unsigned char clientSubId,
                     Timer& simTimer,
                     Timer& realTimer,
                     const ContentTierManager& contentTierManager,
                     ServerMetrics* serverMetrics)
{
    (void)allowList;
    (void)contentTierManager;

    // +0x00 vtable（由编译器插入）
    // +0x04 ~ +0x10 基础成员
    m_callbacks       = &callbacks;
    m_eventing        = &eventing;
    m_resourceLoader  = nullptr;
    m_structureManager= nullptr;

    // +0x14 从 app 获取 game module server
    m_gameModuleServer = nullptr;

    // +0x18 控制块
    m_entityRegistryCtrl = nullptr;

    // +0x1c ~ +0x24
    m_permissionsFile = permissionsFile;
    m_filePathManager = filePathManager;
    m_entityRegistry  = nullptr;

    // +0x28 构造 Timer 子对象（反汇编调用 0x3388420）
    // 真实项目里是 Timer 的构造函数

    // +0x34 chrono duration
    m_duration = duration;

    // +0x38 ~ +0x4c
    m_serverMetrics = serverMetrics;
    m_leaveGameFlag = false;
    m_lastFrameTime = std::chrono::steady_clock::time_point{};
    m_prevFrameTime = std::chrono::steady_clock::time_point{};

    // +0x50 ~ +0x54 FileAccess 相关
    m_fileAccess    = nullptr;
    m_fileAccessEnd = nullptr;

    // +0x58 ~ +0x5c
    m_commands = nullptr;
    m_session  = nullptr;

    // +0x60 ~ +0x6c 各类 Timer / 网络指针
    m_simTimer       = &simTimer;
    m_realTimer      = &realTimer;
    m_networkHandler = &networkHandler;
    m_packetSender   = &packetSender;

    // +0x70 IMinecraftApp 指针
    m_app = &app;

    // +0x74 client sub id
    m_clientSubId = clientSubId;

    // +0x78 EntityRegistry 构造
    // 反汇编调用 0x34b7e20
    // new (m_entityRegistryObj) EntityRegistry();
}

// ===========================================================================
// 析构函数
// ===========================================================================
Minecraft::~Minecraft() {
    // +0x5c session 释放
    if (m_session) {
        // std::shared_ptr reset
        m_session = nullptr;
    }
    // +0x7c 另一份 shared_ptr
    // ... 依次释放

    // +0x5c 再次释放（反汇编中出现两次）
    m_session = nullptr;

    // +0x58 命令对象
    m_commands = nullptr;

    // +0x28 处的 Timer 子对象
    // if (byte[m_simTimeObj] & 1) { /* 释放 */ }

    // +0x24 EntityRegistry
    m_entityRegistry = nullptr;

    // +0x18 控制块
    if (m_entityRegistryCtrl) {
        // __shared_weak_count::__release_weak
        m_entityRegistryCtrl = nullptr;
    }

    // +0x10 / +0x0c
    m_structureManager = nullptr;
    m_resourceLoader   = nullptr;
}

void Minecraft::operator delete(void* p) {
    ::operator delete(p);
}

// ===========================================================================
// resetGameSession
// ===========================================================================
void Minecraft::resetGameSession() {
    if (m_session) {
        // 释放旧 session
        m_session = nullptr;
    }
}

// ===========================================================================
// clearThreadCallbacks
// ===========================================================================
void Minecraft::clearThreadCallbacks() {
    if (m_resourceLoader) {
        // 调用 ResourceLoader::clearThreadCallbacks()
    }
}

// ===========================================================================
// updateScreens
// ===========================================================================
void Minecraft::updateScreens() {
    // 反汇编对 +0x04 处对象调用虚函数
    if (m_callbacks) {
        // m_callbacks->updateScreens();
    }
}

// ===========================================================================
// initAsDedicatedServer
// ===========================================================================
void Minecraft::initAsDedicatedServer() {
    // 反汇编里创建了两个 0x2c 字节的对象，
    // 分别塞到两个全局注册点。
    // 下面是语义等价骨架：
    //
    // auto obj1 = std::make_shared<DedicatedServerConfig>();
    // auto obj2 = std::make_shared<DedicatedServerConfig>();
    // registerDedicatedServer(obj1);
    // registerDedicatedServer2(obj2);
    //
    // 最后调用 initAsDedicatedServerImpl();

    // +0x70 处对象（IMinecraftApp）的虚函数
    if (m_app) {
        // m_app->onDedicatedServerInit();
    }

    // 反汇编最后调用 0x339fc30
    // 这是 Minecraft::initAsDedicatedServerImpl 之类的
}

// ===========================================================================
// init
// ===========================================================================
void Minecraft::init() {
    // 反汇编：
    //   1. 把当前时间写入 +0x48
    //   2. 若全局标志已设置，则跳过
    //   3. 从 +0x70 处对象取 game module server
    //   4. 分配 0x40 字节的对象挂到 +0x58
    m_lastFrameTime = std::chrono::steady_clock::now();

    static bool g_initialized = false;
    if (g_initialized) {
        return;
    }

    // 从 m_app 获取 game module server
    if (m_app) {
        // m_gameModuleServer = m_app->getGameModuleServer();
    }

    // 分配命令对象
    // m_commands = new Commands();

    g_initialized = true;

    // 两个 init 钩子
    // initHook1();
    // initHook2();
}

// ===========================================================================
// getGameModuleServer
// ===========================================================================
GameModuleServer* Minecraft::getGameModuleServer() {
    return reinterpret_cast<GameModuleServer*>(m_gameModuleServer);
}

// ===========================================================================
// initCommands
// ===========================================================================
void Minecraft::initCommands() {
    // 反汇编：
    //   1. 从 +0x70 取 app，调用其虚函数获取 commands
    //   2. 分配 0x40 字节对象挂到 +0x58
    if (m_app) {
        // void* cmds = m_app->getCommands();
        // m_commands = new Commands();
    }
}

// ===========================================================================
// isInitialized
// ===========================================================================
bool Minecraft::isInitialized() const {
    static bool g_initialized = false;
    return g_initialized;
}

// ===========================================================================
// startLeaveGame
// ===========================================================================
void Minecraft::startLeaveGame(bool fade) {
    // 反汇编：
    //   1. 释放 +0x10 处对象
    //   2. 若 +0x5c session 非空，处理
    //   3. 若 fade 为真，额外处理
    //   4. 调用 +0x04 处对象的虚函数（+0x18 / +0x1c）
    if (m_structureManager) {
        // m_structureManager->release();
    }

    if (m_session) {
        // session->stop();
        if (fade) {
            // m_simTimer->pause();
            // m_realTimer->pause();
            // m_networkHandler->stop();
        }
        // m_callbacks->onLeaveGameStart();
    }

    if (m_callbacks) {
        // m_callbacks->onStartLeaveGame(fade);
    }
}

// ===========================================================================
// isLeaveGameDone
// ===========================================================================
bool Minecraft::isLeaveGameDone() const {
    if (!m_session) {
        return true;
    }
    // return m_session->isDone();
    return false;
}

// ===========================================================================
// clientReset
// ===========================================================================
void Minecraft::clientReset() {
    if (m_session) {
        // m_callbacks->onClientReset();
        m_session = nullptr;
    }

    if (m_callbacks) {
        // m_callbacks->onClientResetFinal();
    }
}

// ===========================================================================
// setGameModeReal
// ===========================================================================
void Minecraft::setGameModeReal(GameType gameType) {
    if (!m_session) {
        if (m_callbacks) {
            // m_callbacks->setGameMode(gameType);
        }
        return;
    }

    // m_session->player->setGameMode(gameType);
}

// ===========================================================================
// update
//
// 主循环：处理时间、更新屏幕、tick 模拟/实时、处理网络、事件。
// 反汇编里的循环和 vector 处理被简化为等价骨架。
// ===========================================================================
void Minecraft::update() {
    // 反汇编：
    //   1. 检查 +0x5c session 是否有效
    //   2. 处理 +0x60 和 +0x64 两个 Timer
    //   3. 处理实时 tick、模拟 tick
    //   4. 处理 session 列表（vector）
    //   5. 触发事件
    if (!m_session) {
        return;
    }

    // 处理主循环
    auto now = std::chrono::steady_clock::now();

    // 更新 +0x60 处 Timer
    if (m_simTimer) {
        // m_simTimer->advanceTime(now);
    }

    // 更新 +0x64 处 Timer
    if (m_realTimer) {
        // m_realTimer->advanceTime(now);
    }

    // 计算上一帧时间
    m_prevFrameTime = m_lastFrameTime;
    m_lastFrameTime = now;

    // 处理 session 列表
    // 反汇编里对每个 session 调用 update
    // for (auto& s : sessionList) { s->update(); }

    // 更新屏幕
    updateScreens();

    // 处理事件
    if (m_eventing) {
        // m_eventing->tick();
    }

    // 处理网络
    if (m_networkHandler) {
        // m_networkHandler->tick();
    }
}

// ===========================================================================
// getServerNetworkHandler
// ===========================================================================
NetworkHandler* Minecraft::getServerNetworkHandler() {
    if (!m_session) {
        return nullptr;
    }
    return m_networkHandler;
}

// ===========================================================================
// disconnectClient
// ===========================================================================
void Minecraft::disconnectClient(const NetworkIdentifier& id,
                                 const std::string& reason) {
    if (!m_session) {
        return;
    }
    NetworkHandler* handler = getServerNetworkHandler();
    if (handler) {
        // handler->disconnectClient(id, reason);
    }
}

// ===========================================================================
// tickSimtime
// ===========================================================================
void Minecraft::tickSimtime(int a, int b) {
    (void)a;
    (void)b;

    if (!m_session) {
        return;
    }

    if (m_simTimer) {
        // if (m_simTimer->isPaused()) return;
    }

    if (m_networkHandler) {
        // m_networkHandler->tick();
    }

    // m_session->player->tickSimtime();
    // m_session->level->tickSimtime();
    // m_session->entityRegistry->tick();
}

// ===========================================================================
// tickRealtime
// ===========================================================================
void Minecraft::tickRealtime(int a, int b) {
    if (m_leaveGameFlag) {
        m_leaveGameFlag = false;
        if (m_callbacks) {
            // m_callbacks->onTickRealtimePre();
        }
    }

    if (m_callbacks) {
        // m_callbacks->tickRealtime(a, b);
    }
}

// ===========================================================================
// getLevel
// ===========================================================================
Level* Minecraft::getLevel() const {
    if (!m_session) {
        return nullptr;
    }
    // return m_session->level;
    return nullptr;
}

// ===========================================================================
// setSimTimePause
// ===========================================================================
void Minecraft::setSimTimePause(bool paused) {
    float value = paused ? 0.0f : 1.0f;

    if (m_simTimer) {
        // m_simTimer->setPaused(paused);
    }
    if (m_realTimer) {
        // m_realTimer->setPaused(paused);
    }

    if (m_session) {
        // m_session->paused = paused;
    }

    (void)value;
}

// ===========================================================================
// setSimTimeScale
// ===========================================================================
void Minecraft::setSimTimeScale(float scale) {
    if (!m_simTimer) {
        return;
    }

    if (scale == 0.0f) {
        // 反汇编直接跳到这里
        // m_simTimer->setScale(scale);
        return;
    }

    // 反汇编：
    //   f = m_simTimer->getScale();
    //   m_simTimer->setScale(scale * f);
    (void)scale;
}

// ===========================================================================
// getSimPaused
// ===========================================================================
bool Minecraft::getSimPaused() const {
    if (!m_simTimer) {
        return false;
    }
    // return m_simTimer->isPaused();
    return false;
}

// ===========================================================================
// isModded
// ===========================================================================
bool Minecraft::isModded() {
    return false;
}

// ===========================================================================
// isOnlineClient
// ===========================================================================
bool Minecraft::isOnlineClient() {
    if (!m_session) {
        return false;
    }
    // return m_session->isOnline();
    return false;
}

// ===========================================================================
// getNetworkStatistics
// ===========================================================================
void Minecraft::getNetworkStatistics() const {
    if (m_networkHandler) {
        // m_networkHandler->getStatistics();
    }
}

// ===========================================================================
// hostMultiplayer
//
// 托管多人游戏：构造 ServerNetworkHandler，绑定 session。
// 反汇编里涉及大量参数传递和 unique_ptr 转移，这里做等价还原。
// ===========================================================================
void Minecraft::hostMultiplayer(
    const std::string& worldName,
    std::unique_ptr<Level> level,
    Player* player,
    const mce::UUID& uuid,
    std::unique_ptr<NetEventCallback> netCallback,
    int a, bool b, bool c,
    const std::vector<std::string>& packs,
    std::string connectionStr,
    const ConnectionDefinition& connDef,
    const std::unordered_map<
        PackIdVersion,
        std::string,
        PackIdVersionHash,
        PackIdVersionEq>& packMap,
    Scheduler& scheduler,
    TextFilteringProcessor* filter)
{
    (void)worldName;
    (void)player;
    (void)uuid;
    (void)a;
    (void)c;
    (void)packs;
    (void)connectionStr;
    (void)connDef;
    (void)packMap;
    (void)scheduler;
    (void)filter;

    // 1. 释放旧 session
    if (m_session) {
        // m_callbacks->onSessionEnd();
        m_session = nullptr;
    }

    // 2. 通知
    if (m_callbacks) {
        // m_callbacks->onHostMultiplayerStart();
    }

    // 3. 从 +0x68 取网络处理器
    if (m_networkHandler) {
        // m_networkHandler->startHosting(...);
    }

    // 4. 构造新的 session
    if (m_app) {
        // m_app->createServerNetworkHandler(...);
    }

    // 5. 若 netCallback 有效，转移所有权
    if (netCallback) {
        // startClientGame(std::move(netCallback));
    }

    // 6. 若 level 有效，设置到 session
    if (level) {
        // m_session->level = std::move(level);
    }

    // 7. 最后根据 b 决定是否调用 +0x04 的虚函数
    if (b) {
        // m_callbacks->onHostMultiplayerEnd();
    }
}

// ===========================================================================
// getClientSubId
// ===========================================================================
unsigned char Minecraft::getClientSubId() const {
    return m_clientSubId;
}

// ===========================================================================
// setupServerCommands
// ===========================================================================
void Minecraft::setupServerCommands(const std::string& a, const std::string& b) {
    if (!m_session) {
        return;
    }

    // 反汇编：
    //   1. 调用 +0x20 处对象虚函数
    //   2. 从 +0x14 取 game module server
    //   3. 用 commands 初始化
    if (m_commands) {
        // setupCommands(m_commands, a, b);
    }
}

// ===========================================================================
// usesNonLocalConnection
// ===========================================================================
bool Minecraft::usesNonLocalConnection(const NetworkIdentifier& id) {
    if (!m_networkHandler) {
        return false;
    }
    // return !m_networkHandler->isLocalConnection(id);
    (void)id;
    return true;
}

// ===========================================================================
// getNetworkHandler
// ===========================================================================
NetworkHandler* Minecraft::getNetworkHandler() {
    return m_networkHandler;
}

// ===========================================================================
// startClientGame
// ===========================================================================
void Minecraft::startClientGame(std::unique_ptr<NetEventCallback> cb) {
    // 释放旧 session
    if (m_session) {
        m_session = nullptr;
    }

    // 反汇编分配一个 0x1c 字节的对象
    // 里面存 {networkHandler, packetSender, netCallback, clientSubId}
    struct ClientGameSession {
        NetworkHandler* networkHandler;
        void* unk1;
        void* unk2;
        NetEventCallback* netCallback;
        void* unk3;
        PacketSender* packetSender;
        unsigned char clientSubId;
    };

    void* raw = ::operator new(0x1c);
    ClientGameSession* session = reinterpret_cast<ClientGameSession*>(raw);
    session->networkHandler = m_networkHandler;
    session->unk1 = nullptr;
    session->unk2 = nullptr;
    session->netCallback = cb.release();
    session->unk3 = nullptr;
    session->packetSender = m_packetSender;
    session->clientSubId = m_clientSubId;

    // 保存旧 session 并替换
    if (m_session) {
        // 释放旧 session
    }
    m_session = session;

    // 反汇编调用 0x3541540（ClientGameSetup 之类）
    if (m_networkHandler) {
        // setupClientGame(m_networkHandler, session);
    }
}

// ===========================================================================
// joinWorldInProgress
// ===========================================================================
void Minecraft::joinWorldInProgress(std::unique_ptr<NetEventCallback> cb) {
    NetEventCallback* raw = cb.release();
    if (raw) {
        // m_session->netCallback = raw;
    }

    // 调用 0x346b750（joinWorldInProgressImpl）
    if (m_session) {
        // joinWorldInProgressImpl(m_session);
    }

    if (m_session) {
        // session->onJoinWorld();
    }
}

// ===========================================================================
// activateAllowList
// ===========================================================================
void Minecraft::activateAllowList() {
    if (!m_session) {
        return;
    }

    // 反汇编：从 session 取 allowList 并激活
    if (m_session) {
        // m_session->allowList->activate();
    }
}

// ===========================================================================
// getCommands
// ===========================================================================
void* Minecraft::getCommands() {
    return m_commands;
}

// ===========================================================================
// hasCommands
// ===========================================================================
bool Minecraft::hasCommands() {
    return m_commands != nullptr;
}

// ===========================================================================
// getNetEventCallback
// ===========================================================================
NetEventCallback* Minecraft::getNetEventCallback() {
    if (!m_session) {
        return nullptr;
    }
    // return m_session->netCallback;
    return nullptr;
}

// ===========================================================================
// getServerLocator
// ===========================================================================
NetworkHandler* Minecraft::getServerLocator() {
    if (!m_networkHandler) {
        return nullptr;
    }
    // return m_networkHandler->getServerLocator();
    return nullptr;
}

// ===========================================================================
// getServerLocator (const)
// ===========================================================================
const NetworkHandler* Minecraft::getServerLocator() const {
    if (!m_networkHandler) {
        return nullptr;
    }
    // return m_networkHandler->getServerLocator();
    return nullptr;
}

// ===========================================================================
// getNetworkHandler (const)
// ===========================================================================
NetworkHandler* Minecraft::getNetworkHandler() const {
    return m_networkHandler;
}

// ===========================================================================
// getEventing
// ===========================================================================
IMinecraftEventing* Minecraft::getEventing() const {
    return m_eventing;
}

// ===========================================================================
// onClientCreatedLevel
// ===========================================================================
void Minecraft::onClientCreatedLevel(std::unique_ptr<Level> level) {
    Level* raw = level.release();
    if (m_session) {
        // m_session->level = raw;
    }
}

// ===========================================================================
// getTimer
// ===========================================================================
Timer* Minecraft::getTimer() {
    return m_simTimer;
}

// ===========================================================================
// getLastTimestep
// ===========================================================================
double Minecraft::getLastTimestep() {
    if (!m_realTimer) {
        return 0.0;
    }
    // return m_realTimer->lastTimestep;
    // 反汇编从 +0x1c 读取 float 并转为 double
    return 0.0;
}

// ===========================================================================
// getResourceLoader
// ===========================================================================
ResourceLoader* Minecraft::getResourceLoader() {
    return m_resourceLoader;
}

// ===========================================================================
// getStructureManager
// ===========================================================================
StructureManager* Minecraft::getStructureManager() {
    return m_structureManager;
}

// ===========================================================================
// getEntityRegistry
// ===========================================================================
EntityRegistry* Minecraft::getEntityRegistry() {
    return &m_entityRegistryObj;
}