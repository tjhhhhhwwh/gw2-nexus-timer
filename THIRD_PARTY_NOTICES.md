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
- 小程序的24张 BOSS 原图已复制到 `assets/bosses/b01.jpg` 至 `b24.jpg`，与其资产清单中的 `boss01.jpg` 至 `boss24.jpg` 逐字节一致（总计750,516字节）。
- 图片在构建时嵌入 DLL，不保留运行时图片链接；不包含 COS 访问配置或密钥。
- 本次只是复用原数据，没有逐项重新核验游戏内时刻，也不额外授予游戏素材或上游数据的权利。

中文字体使用玩家 Windows 安装中的字体，通过 Nexus 字体服务加载；仓库不分发字体文件。

## 世界事件时间轴

- 来源：星岬岛事件时间表 https://gw2.wishingstarmoye.com/gw2timer
- 导入日期：2026-11-24；页面版本 `v1.20261110`。
- 数据文件：https://gw2.wishingstarmoye.com/la-assets/gw2timer/gw2timer_new_data_20261006.js
- 源数据 SHA256：`2677fdae6ec85e8abc84aeacd2cd07dc94dbc9c906d1639bf5bb4d48778cc33c`。
- 快照：`data/events.json`；生成头文件：`src/event_data.generated.h`；42条时间轴、150个阶段定义，保留中文标签、阶段时长、颜色、传送点和轮换规则，补充英文描述。
- 分类快捷栏及成员参考页面内 `filters`：所有、永恒愿景、珍瑟、天界之谜、凯珊、泰瑞亚、迈古玛、烈焰征途、冰巢传说、特殊事件、一条龙；同一事件可出现在多个快捷分类。
- 与页面当时的显示一致，排除禁用的四风节、斗龙节、万圣节与已注释的裂隙事件；不将其视为可用日程。
- 来源以00:00 UTC为日界线；插件显示为北京时间 UTC+8，并在08:00切换每周轮换。迷雾武器商人按来源2018-05-10 00:00 UTC锚点计算；血石目击者仅为推测时段，不保证出现。
- DLL使用本地数据与自行编写的原生 ImGui 界面；不包含网站渲染脚本，不嵌入浏览器。英文标签为描述性翻译，未声称全部为官方译名。
- 本记录只说明数据来源，不额外授予游戏素材、网站或上游数据的权利。日程不能判断地图实例真实状态。

## 微信小程序入口

- `assets/miniapp/entry.jpg`：用户于2026-11-24提供的微信小程序码原图，1280×1280、327,552字节。
- SHA256：`7198f19f0e13453f8a12e0a8ec5b1b347f4a043ab2478c644157f31d0468066e`。
- 原图逐字节嵌入 DLL，仅在「关于作者」页显示，保留完整正方形比例，不裁剪或重绘码点；可点击放大。
- 小程序功能简介依据本地 `gw2-tool/README.md` 中已有模块编写：日常/周常、计时、资料检索、配装、传奇与攻略。此修改不更新或上传小程序工程。
