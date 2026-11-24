# 第三方来源与数据记录

## Nexus API

- 项目：https://github.com/RaidcoreGG/Nexus-API
- 固定提交：`9b2c53df86c00db6495642bfcff2d0611bd957ef`
- API：6
- 文件：`third_party/nexus/Nexus.h`
- 许可：MIT，完整文本保留在 `third_party/nexus/LICENSE`。

插件入口参考 Nexus 官方模板的 GetAddonDef / Load / Unload 接口结构：
https://github.com/RaidcoreGG/Nexus-Template-cpp

## Dear ImGui（Nexus 兼容分支）

- 项目：https://github.com/RaidcoreGG/imgui18000
- 固定提交：`58075c4414b985b352d10718b02a8c43f25efd7c`
- 版本：1.80，与官方模板固定提交相同。
- 文件：`third_party/imgui/` 中的核心源码与头文件。
- 许可：MIT，完整文本保留在 `third_party/imgui/LICENSE.txt`。

两项依赖直接保存到仓库，便于从 Gitee 克隆后离线构建，不需要访问 GitHub 子模块。

## 世界BOSS数据

- 本地来源：现有 `gw2-tool/data/bosses.js`，快照日期 2026-11-06。
- 导入日期：2026-11-24。
- 内容：24项首领/大型事件，246个每日时刻；名称、地图、区域和传送点代码。
- 数据保存为 `data/bosses.json`；生成的 C++ 位于 `src/boss_data.generated.h`。
- 不包含小程序中的配图、COS访问配置或密钥。
- 本次只是复用原数据，没有逐项重新核验游戏内时刻，也不额外授予游戏素材或上游数据的权利。

中文字体使用玩家 Windows 安装中的字体，通过 Nexus 字体服务加载；仓库不分发字体文件。
