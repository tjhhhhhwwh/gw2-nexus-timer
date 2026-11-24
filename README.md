# 激战2世界计时 · Nexus 插件

第一版定位：在游戏内查看世界BOSS计划刷新时间，复制名称与传送点。

源码仓库：https://gitee.com/iamapsycho/gw2-nexus-timer

GitHub / Releases：https://github.com/tjhhhhhwwh/gw2-nexus-timer

## v0.1.0 功能

- CN / EN 界面切换，首领名、地图名、日期提示和复制文本同步切换，记住语言选择。
- 悬浮面板按下次计划刷新时间排序，实时显示倒计时。
- 点击首领名称展开全天时刻；支持按名称、地图筛选。
- 一键复制「名称 · 今天/明天 HH:mm · 地图 传送点代码」。由玩家粘贴到游戏聊天框。
- `Ctrl+Shift+T` 显示或隐藏；可在 Nexus 快捷键管理里修改，也可从插件选项重新打开面板。
- 固定使用北京时间（UTC+8），不随电脑显示时区改变；依赖本机系统时钟准确。
- 数据沿用现有小程序世界BOSS页：24项首领/大型事件、每日246个时刻。原列表也含苍翠夜晚、四门等地图事件，本版保持原列表。

固定时刻表不能判定玩家所在地图实例的事件是否成功触发、结束或已被击杀。面板显示的是计划刷新时间。

## 当前交付状态

- 计时核心、数据快照、Nexus API 6 / ImGui 1.80 接入源码和 Windows x64 构建工程已建立。
- 默认已配置 Nexus GitHub 更新源；只有对应的可识别 Release 与 DLL 资产实际发布后，才能完成在线更新闭环。
- 当前开发环境是 macOS；计时核心测试不等于已完成游戏内兼容性验证。
- 已使用 Zig 0.14.1 在 macOS 交叉编译 Windows x64 DLL；Visual Studio 原生构建及国服游戏内加载、中文字体、剪贴板与卸载仍需在对应环境验证。

## Windows 构建

安装 Visual Studio 2022 的「使用 C++ 的桌面开发」（含 Windows SDK）及 CMake 3.20+。
依赖已随仓库固定保存，不需要额外从 GitHub 下载子模块。

在仓库目录运行 PowerShell：

```powershell
powershell -ExecutionPolicy Bypass -File tools/build-windows.ps1
```

脚本会构建 x64 Release、运行计时测试并将 DLL 复制到 `dist/gw2_nexus_timer.dll`，输出 SHA256。

也可手动构建：

```powershell
cmake -S . -B build/windows -G "Visual Studio 17 2022" -A x64
cmake --build build/windows --config Release
ctest --test-dir build/windows -C Release --output-on-failure
```

## 安装与卸载

1. 先安装 [Nexus](https://raidcore.gg/Nexus)，单独确认它能在目标客户端中正常加载。
2. 退出游戏，将构建出的 `gw2_nexus_timer.dll` 放到游戏目录下的 `addons/`。
3. 启动游戏，在 Nexus 插件管理中检查并启用插件。
4. 默认打开世界计时，`Ctrl+Shift+T` 可切换显示。中文字体通过 Nexus 字体服务加载 Windows 自带的微软雅黑或黑体，不打包字体文件。
5. 升级时退出游戏并替换 DLL；卸载时退出游戏删除该 DLL。

面板和 Nexus 插件选项均提供 `Language: CN / EN`。语言选择保存在
`addons/gw2-nexus-timer/settings.ini`。切换语言不会改变时刻表或时区；两种模式均显示北京时间。

不要将插件 DLL 改名为 `d3d11.dll`；那是 Nexus 加载器的位置。

## 更新数据

插件仓库独立保存 `data/bosses.json`，运行时不依赖相邻的小程序目录。
修改时刻或传送点后生成头文件，再构建插件：

```sh
python3 tools/generate_data.py
python3 tools/generate_data.py --check
```

当前快照日期为 2026-11-06，来自小程序已有数据；本次没有重新核验游戏内全部时刻。
数据与第三方依赖来源见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## macOS / Linux 验证计时核心

```sh
cmake -S . -B build/core
cmake --build build/core
ctest --test-dir build/core --output-on-failure
```

没有 CMake 时可使用系统 C++ 编译器：

```sh
c++ -std=c++17 -Wall -Wextra -Wpedantic -Isrc src/schedule.cpp tests/schedule_tests.cpp -o /tmp/gw2-timer-tests
/tmp/gw2-timer-tests
```

测试覆盖 UTC+8、跨午夜、恰好刷新时刻、排序、中文复制文本以及完整24项数据。

### 可选：交叉编译 Windows DLL

安装 [uv](https://docs.astral.sh/uv/) 后运行：

```sh
python3 tools/build-cross.py
```

脚本通过 uv 使用固定的 Zig 0.14.1 工具链，首次运行需要下载工具链。
输出 `dist/gw2_nexus_timer.dll` 及 SHA256；可传入 `--github-repository OWNER/REPOSITORY`。
交叉编译成功仅证明 DLL 可生成，不等于已在 Windows / 国服游戏内完成验收。

## Gitee 与更新发布

Gitee 用于源码、版本标签和发布文件管理。Nexus API 当前提供的更新方式：

| Provider | 方式 |
| --- | --- |
| `UP_Raidcore` | Raidcore 托管 |
| `UP_GitHub` | GitHub Releases |
| `UP_Direct` | 直接下载 DLL 的链接 |
| `UP_Self` | 插件自行检查更新，并请求 Nexus 下载 |
| `UP_None` | 关闭自动更新，手动安装 |

没有 Gitee 专用 Provider。当前默认使用 `UP_GitHub`，指向 `tjhhhhhwwh/gw2-nexus-timer`。
Gitee 继续管理源码，GitHub Releases 承载 DLL；发布后的发现更新、下载和替换仍需实测。
Gitee 仓库首页、需要登录的附件页面和 Raw 源码页面都不能直接当作 DLL 下载链接。

### GitHub Releases

已准备 `.github/workflows/build.yml`，代码同步到 GitHub 后会构建 Windows DLL 并生成构建产物，不会自动发布 Release。
在 GitHub Actions 中手动运行 `Prepare release draft`，可构建并创建附带 DLL 的 v0.1.0 预发布草稿，供检查后发布。
默认更新仓库已经配置；也可在构建时显式指定：

```powershell
./tools/build-windows.ps1 -GitHubRepository "tjhhhhhwwh/gw2-nexus-timer"
```

如需禁用自动更新，可传入 `-GitHubRepository ""`（交叉构建对应 `--github-repository ""`）。
Nexus 按 Release 标签比较版本，并选择 DLL 资产；该仓库每个 Release 只附一个插件 DLL。
草稿不会出现在匿名更新检查中，预发布版本需要玩家允许预发布更新。
在 GitHub 创建 Release 并上传 DLL 后，仍需实测 Nexus 的发现更新、下载与替换流程；配置参数本身不代表发布已完成。

## 游戏内验收

- 确认国服客户端能加载 Nexus 和该插件，检查是否出现日志错误。
- 检查中文名称、全天时刻、窗口收起与重新打开、修改快捷键。
- 检查 CN / EN 切换、英文名称/地图/复制文本，以及重启后语言选择。
- 在午夜前后检查「今天/明天」和下次刷新；切换电脑显示时区后结果应仍是北京时间。
- 点击复制后手动粘贴到游戏聊天框，检查中文和传送点代码。
- 反复加载/卸载，检查字体、快捷键和渲染回调是否正常清理。

公开发布前确认插件 Signature 的唯一性；当前使用开发阶段标识。
