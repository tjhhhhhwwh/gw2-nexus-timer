# 激战2世界计时 · Nexus 插件

在游戏内查看世界BOSS计划刷新时间与世界事件时间轴，复制名称与传送点。

源码仓库：https://gitee.com/iamapsycho/gw2-nexus-timer

GitHub / Releases：https://github.com/tjhhhhhwwh/gw2-nexus-timer

## v0.1.5 功能

- 默认界面字体由约12像素提高到18像素，时间轴与底部说明同步放大，提高说明文字对比度。BOSS状态标签20像素、时间与地图18像素，名称与计时数字一并放大；悬停查看完整信息。
- 默认窗口仍为798×595；调整卡片行高、按钮和时间轴间距以容纳大字，窄窗口自动换行。
- 「世界BOSS / 事件时间轴 / 关于作者」三个标签页；关于页包含作者信息、小程序简介和微信小程序码，支持点击放大扫码。小程序码嵌入 DLL，无需外置图片。
- 事件时间轴导入星岬岛2026-11-24快照，包含42条非节日时间轴、150个阶段定义；数据随 DLL 离线提供，不嵌入网页。
- 分类快捷栏参考星岬岛：所有、永恒愿景、珍瑟、天界之谜、凯珊、泰瑞亚、迈古玛、烈焰征途、冰巢传说、特殊事件、一条龙。彩色描边、选中高亮、窄窗口自动换行；筛选成员与来源一致，特殊事件/一条龙允许跨分类重叠。
- 时间轴支持2小时/4小时/24小时范围、前后翻页、回到现在、双语名称/阶段筛选；红线标记当前北京时间，金框表示当前计划阶段。悬停查看完整阶段和时段，点击活动阶段复制事件与传送点。
- 源数据以 UTC 日界线编排，显示统一转换为 UTC+8；每周轮换在北京时间08:00切换，保留商人轮换和跨日连续阶段。血石目击者明确标为推测时段、不保证刷新。
- CN / EN 界面切换，首领名、地图名、日期提示和复制文本同步切换，记住语言选择。
- 双列大图卡片，窗口较窄时自动切成单列；大号首领名称、地图、传送点与计时。
- 红色「计划进行中」显示计划开始后15分钟内的已过时间；黄色「即将开始」为15分钟内开始，灰色「尚未开始」为更晚时刻。这个展示窗口不能判定地图实例真实状态。
- 计划开始后15分钟内的首领优先展示，其余按下次计划刷新时间排序；已开始的卡片标注「本次」，其余标注「下次」。
- 24张 BOSS 原图嵌入 DLL，按比例裁切填满图片区域；无需另外安装图片目录。
- 点击图片或首领名称查看全天时刻；支持按名称、地图筛选。
- 点击地图与传送点复制「名称 · 昨天/今天/明天 HH:mm · 地图 传送点代码」。由玩家粘贴到游戏聊天框。
- `Ctrl+Shift+T` 显示或隐藏；可在 Nexus 快捷键管理里修改，也可从插件选项重新打开面板。
- 固定使用北京时间（UTC+8），不随电脑显示时区改变；依赖本机系统时钟准确。
- 数据沿用现有小程序世界BOSS页：24项首领/大型事件、每日246个时刻。原列表也含苍翠夜晚、四门等地图事件，本版保持原列表。

固定时刻表不能判定玩家所在地图实例的事件是否成功触发、结束或已被击杀。面板显示的是计划刷新时间。

## 当前交付状态

- 计时核心、数据快照、Nexus API 6 / ImGui 1.80 接入源码和 Windows x64 构建工程已建立。
- 默认已配置 Nexus GitHub 更新源；只有对应的可识别 Release 与 DLL 资产实际发布后，才能完成在线更新闭环。
- v0.1.5 已在 Windows x64 使用 Visual Studio 2026 原生构建，5项 CTest 通过；独立 ImGui 渲染检查涵盖中文/英文、宽/窄窗口、三标签、分类筛选、范围选择、卡片弹窗和小程序码放大。
- 国服游戏内加载、Nexus 字体回调、剪贴板与卸载仍需在真实客户端验证，独立渲染预览不等于客户端验收。

## Windows 构建

安装 Visual Studio 2022 / 2026 的 C++ 构建工具（含 Windows SDK）、支持相应版本的 CMake 和 Python 3。
依赖已随仓库固定保存，不需要额外从 GitHub 下载子模块。

在仓库目录运行 PowerShell：

```powershell
powershell -ExecutionPolicy Bypass -File tools/build-windows.ps1
```

脚本会检查数据、自动选择已安装的 Visual Studio，构建 x64 Release、运行测试并将 DLL 复制到 `dist/gw2_nexus_timer.dll`，输出 SHA256。也可用 `-Generator` 指定 CMake 生成器。DLL 静态链接 MSVC 运行库。

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

插件仓库独立保存 `data/bosses.json` 与 `data/events.json`，运行时不依赖相邻的小程序目录或网页。
修改时刻或传送点后生成头文件，再构建插件：

```sh
python3 tools/generate_data.py
python3 tools/generate_data.py --check
python3 tools/generate_events.py
python3 tools/generate_events.py --check
```

BOSS快照日期为2026-11-06，来自小程序已有数据；本次没有改动其时刻。
事件时间轴快照日期为2026-11-24，来源为[星岬岛事件时间表](https://gw2.wishingstarmoye.com/gw2timer)，页面版本 `v1.20261110`、数据文件 `gw2timer_new_data_20261006.js`。未导入页面当时禁用的四风节、斗龙节、万圣节或已注释的事件；快照不会自动联网更新。
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

测试覆盖 UTC+8、跨午夜、恰好刷新时刻、排序、双语复制文本、完整24项BOSS数据与42条事件时间轴、7天阶段覆盖、每周与商人轮换、11组快捷筛选，以及本地 JPEG 完整性和嵌入图片逐字节一致性。

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
- 断网检查24个名称旁的缩略图；图片应保持比例，加载期间名称和复制按钮仍可使用。
- 检查 CN / EN 切换、英文名称/地图/复制文本，以及重启后语言选择。
- 在午夜前后检查「今天/明天」和下次刷新；切换电脑显示时区后结果应仍是北京时间。
- 点击复制后手动粘贴到游戏聊天框，检查中文和传送点代码。
- 切换三标签与11组分类，检查范围选择、前后翻页/回到现在、红色当前时间线、阶段悬停和事件复制；检查08:00轮换与跨午夜日期。
- 打开「关于作者」，检查简介、完整小程序码、点击放大；用手机微信验证扫码进入。
- 反复加载/卸载，检查字体、快捷键和渲染回调是否正常清理。

公开发布前确认插件 Signature 的唯一性；当前使用开发阶段标识。
