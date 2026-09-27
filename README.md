# MCRe-BedrockEditionCode
> 尝试通过逆向补全 Minecraft Bedrock Edition 1.16.200.02 的源代码  

> [!TIP]
> 该代码由 MFSCelebrate 逆向并遵循 [GNU General Public License](./LICENSE) 协议之后才能进行对该项目的其他操作与管理  
> 作者本人: [MFSCelebrate(Bilibili)](https://b23.tv/hTl7eI5)

> [!WARNING]
> 该仓库使用的大部分cpp文件均为空文件，部分源码内容已在下方表格写出。  
> 逆向工作需要时间，部分内容会尽可能完善

## 背景
该仓库用于存放逆向完毕的 MCBE 1.16.200 源代码，为 [MCRe-NoiseFarlandsJava](https://github.com/MFSCelebrate/MCRe-NoiseFarlandsJava) 边境之地 Mod 的基岩版生成器模拟做基础  

## 逆向状态
此列表从 **2026-09-28 00:43** 开始记录。
| 批次 | 文件 | 功能 | 是否合并 |
|------|------|------|----------|
| 1 | ImprovedNoise.cpp | 改进噪声（ImprovedNoise）实现 | ✅ 是 |
| 2 | PerlinNoise.cpp | Perlin 噪声（PerlinNoise）实现 | ✅ 是 |
| 3 | PerlinSimplexNoise.cpp | Perlin Simplex 噪声实现 | ✅ 是 |
| 4 | SimplexNoise.cpp | Simplex 噪声实现 | ✅ 是 |
| 5 | FeatureToggles.cpp | 开发者版本的特殊按钮实现，在正式版仅有单一功能 | ✅ 是 |
| 6 | OverworldGenerator.cpp | 主世界生成的总调度器，用于组织生物群系层、采样高度场、构建地表方块、生成结构、后处理区块、查询结构和出生点 | ✅ 是 |
| 7 | OverworldGenerator.h | 负责 OverworldGenerator.cpp 的接口、枚举和严格内存布局 | ✅ 是 |
| 8 |  |  |  |

## 许可证
本项目依据 GNU General Public License 协议开源，详见 [LICENSE](./LICENSE) 文件