#include <windows.h>
#include "Nexus.h"
#include "imgui.h"
#include "schedule.h"
#include "clipboard_win.h"
#include "boss_images.generated.h"

#include <atomic>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>

namespace {
constexpr char kWindowCN[] = u8"世界计时 · 世界BOSS###GW2NexusTimer";
constexpr char kWindowEN[] = "World Timer - World Bosses###GW2NexusTimer";
constexpr char kBind[] = "GW2_NEXUS_TIMER_TOGGLE";
constexpr char kFontId[] = "GW2_NEXUS_TIMER_CHINESE";
AddonAPI_t* api = nullptr;
NexusLinkData_t* nexus = nullptr;
std::atomic<ImFont*> chinese_font{nullptr};
std::atomic<bool> visible{true};
bool font_requested = false;
ImGuiTextFilter filter;
int feedback = 0;
world_timer::Language language = world_timer::Language::CN;
std::filesystem::path settings_path;
std::chrono::steady_clock::time_point feedback_until;
std::unordered_map<std::string, std::string> boss_texture_keys;

void LoadBossImages() {
    for (const auto& image : world_timer::kBossImages) {
        auto& key = boss_texture_keys[image.id];
        key = "GW2_TIMER_BOSS_" + std::string(image.id);
        // Nexus decodes the bytes synchronously and owns the resulting texture.
        api->Textures_GetOrCreateFromMemory(key.c_str(), const_cast<unsigned char*>(image.data), image.size);
    }
}

const char* T(const char* cn, const char* en) {
    return language == world_timer::Language::CN ? cn : en;
}

void ReadLanguage() {
    const char* folder = api->Paths_GetAddonDirectory("gw2-nexus-timer");
    if (!folder) return;
    settings_path = std::filesystem::u8path(folder) / "settings.ini";
    std::ifstream file(settings_path);
    std::string line;
    while (std::getline(file, line)) {
        if (line == "language=en") language = world_timer::Language::EN;
        if (line == "language=cn") language = world_timer::Language::CN;
    }
}

void LanguageSelector() {
    int selected = language == world_timer::Language::CN ? 0 : 1;
    ImGui::SetNextItemWidth(110.0f);
    if (!ImGui::Combo("Language###language", &selected, "CN\0EN\0")) return;
    language = selected == 0 ? world_timer::Language::CN : world_timer::Language::EN;
    if (settings_path.empty()) return;
    std::error_code error;
    std::filesystem::create_directories(settings_path.parent_path(), error);
    std::ofstream file(settings_path);
    if (file) file << "language=" << (selected == 0 ? "cn" : "en") << '\n';
    if (!file) api->Log(LOGL_WARNING, "GW2 Nexus Timer", "Could not save language setting.");
}

std::int64_t Now() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void ReceiveFont(const char*, void* font) {
    chinese_font.store(static_cast<ImFont*>(font));
}

void Toggle(const char*, bool release) {
    if (!release) visible.store(!visible.load());
}

void LoadChineseFont() {
    // Nexus owns atlas mutation; never add fonts to its atlas directly.
    // Its font manager builds ranges from localization texts, overriding a
    // font config's GlyphRanges. Register our glyphs so boss names render even
    // when Nexus itself uses English. Queued localization values are copied.
    std::string glyphs = u8"世界计时北京时间项首领大型事件固定时刻表计划刷新时间点击首领查看全天时刻"
        u8"筛选名称地图刷新倒计时操作复制名称与传送点已复制可粘贴到游戏聊天框失败剪贴板可能被占用请重试"
        u8"显示隐藏可在中修改时刻数据以本机系统时钟计算所有时间为今天明天·（）";
    for (const auto& boss : world_timer::Bosses()) glyphs += boss.name + boss.map + boss.zone;
    api->Localization_Set("GW2_NEXUS_TIMER_FONT_GLYPHS", "en", glyphs.c_str());
    wchar_t windows[MAX_PATH]{};
    const UINT length = GetWindowsDirectoryW(windows, MAX_PATH);
    if (!length || length >= MAX_PATH) return;
    for (const auto* filename : {L"msyh.ttc", L"simhei.ttf"}) {
        const auto path = std::filesystem::path(windows) / L"Fonts" / filename;
        std::error_code error;
        if (!std::filesystem::exists(path, error)) continue;
        ImFontConfig config;
        config.GlyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesChineseFull();
        api->Fonts_AddFromFile(kFontId, 17.0f, path.u8string().c_str(), ReceiveFont, &config);
        font_requested = true;
        break;
    }
}

void Render() {
    if (!visible.load() || (nexus && !nexus->IsGameplay)) return;
    ImFont* font = chinese_font.load();
    if (font) ImGui::PushFont(font);
    bool open = visible.load();
    ImGui::SetNextWindowSize(ImVec2(780, 560), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(language == world_timer::Language::CN ? kWindowCN : kWindowEN, &open)) {
        const auto now = Now();
        LanguageSelector();
        ImGui::Text(T(u8"北京时间 %s · %zu 项首领 / 大型事件", "Beijing (UTC+8) %s | %zu bosses / events"), world_timer::ClockText(now).c_str(),
                    world_timer::Bosses().size());
        ImGui::TextDisabled("%s", T(u8"固定时刻表 · 计划刷新时间 · 点击首领查看全天时刻",
            "Scheduled times | Expand a boss for the full daily schedule"));
        if (!font) {
            ImGui::TextWrapped("Chinese font unavailable/loading. Requires Microsoft YaHei or SimHei in Windows Fonts.");
        }
        filter.Draw(T(u8"筛选名称 / 地图###BossFilter", "Name / map###BossFilter"), -160.0f);
        ImGui::Separator();

        // Rebuild at most once a second, rather than calculating schedules every frame.
        static std::int64_t cached_second = -1;
        static std::vector<world_timer::Occurrence> upcoming;
        if (cached_second != now) {
            upcoming = world_timer::Upcoming(world_timer::Bosses(), now);
            cached_second = now;
        }
        const float footer = ImGui::GetTextLineHeightWithSpacing() * 3.0f;
        ImGui::BeginChild("bosses", ImVec2(0, -footer), false);
        if (ImGui::BeginTable("schedule", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH
                                          | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn(T(u8"刷新 / 倒计时", "Next / countdown"), ImGuiTableColumnFlags_WidthFixed, 140);
            ImGui::TableSetupColumn(T(u8"世界BOSS / 地图", "World boss / map"));
            ImGui::TableSetupColumn(T(u8"操作", "Action"), ImGuiTableColumnFlags_WidthFixed, 190);
            ImGui::TableHeadersRow();
            for (const auto& row : upcoming) {
                const auto& boss = *row.boss;
                const auto search = boss.name + " " + boss.map + " " + boss.zone
                    + " " + boss.name_en + " " + boss.map_en + " " + boss.zone_en;
                if (!filter.PassFilter(search.c_str())) continue;
                ImGui::PushID(boss.id.c_str());
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s %s", world_timer::DayLabel(row.starts_at, now, language).c_str(),
                            world_timer::ClockText(row.starts_at).c_str());
                const auto remaining = row.starts_at - now;
                const auto color = remaining <= 900 ? ImVec4(1, 0.75f, 0.25f, 1) : ImVec4(0.7f, 0.8f, 0.85f, 1);
                ImGui::TextColored(color, "%s", world_timer::Countdown(remaining).c_str());
                ImGui::TableNextColumn();
                const auto picture_pos = ImGui::GetCursorScreenPos();
                ImGui::Dummy(ImVec2(48.0f, 48.0f));
                if (auto* picture = api->Textures_Get(boss_texture_keys.at(boss.id).c_str());
                    picture && picture->Resource && picture->Width && picture->Height) {
                    const float scale = 48.0f / static_cast<float>(std::max(picture->Width, picture->Height));
                    const ImVec2 size(picture->Width * scale, picture->Height * scale);
                    const ImVec2 start(picture_pos.x + (48.0f - size.x) * 0.5f,
                                       picture_pos.y + (48.0f - size.y) * 0.5f);
                    ImGui::GetWindowDrawList()->AddImage(picture->Resource, start,
                        ImVec2(start.x + size.x, start.y + size.y));
                }
                ImGui::SameLine();
                ImGui::BeginGroup();
                if (ImGui::TreeNode("daily", "%s", world_timer::BossName(boss, language).c_str())) {
                    ImGui::TextDisabled("%s", T(u8"全天时刻（北京时间）", "Daily schedule (UTC+8)"));
                    for (std::size_t i = 0; i < boss.minutes.size(); ++i) {
                        if (i % 3 != 0) ImGui::SameLine();
                        ImGui::Text("%02d:%02d", boss.minutes[i] / 60, boss.minutes[i] % 60);
                    }
                    ImGui::TreePop();
                }
                ImGui::TextDisabled("%s", world_timer::MapName(boss, language).c_str());
                ImGui::EndGroup();
                ImGui::TableNextColumn();
                if (ImGui::Button(T(u8"复制名称与传送点###copy", "Copy name & waypoint###copy"))) {
                    feedback = CopyUnicodeText(world_timer::CopyText(row, now, language)) ? 1 : 2;
                    feedback_until = std::chrono::steady_clock::now() + std::chrono::seconds(4);
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
        if (std::chrono::steady_clock::now() < feedback_until)
            ImGui::TextWrapped("%s", feedback == 1
                ? T(u8"已复制名称与传送点，可粘贴到游戏聊天框", "Copied. Paste into game chat.")
                : T(u8"复制失败，剪贴板可能被占用，请重试", "Clipboard unavailable. Please try again."));
        ImGui::TextDisabled("%s", T(u8"Ctrl+Shift+T 显示 / 隐藏（可在 Nexus 中修改）",
            "Ctrl+Shift+T to toggle (configurable in Nexus)"));
        ImGui::TextDisabled(T(u8"时刻数据 %s · 以本机系统时钟计算", "Schedule %s | Uses the system clock"), world_timer::SnapshotDate());
    }
    ImGui::End();
    if (!open) visible.store(false);
    if (font) ImGui::PopFont();
}

void Options() {
    ImFont* font = chinese_font.load();
    if (font) ImGui::PushFont(font);
    ImGui::TextUnformatted(T(u8"世界计时 · 世界BOSS v0.1.0", "World Timer - World Bosses v0.1.0"));
    LanguageSelector();
    bool show = visible.load();
    if (ImGui::Checkbox(T(u8"显示世界计时###show", "Show world timer###show"), &show)) visible.store(show);
    ImGui::TextWrapped("%s", T(u8"查看世界BOSS计划刷新时间，复制名称与传送点。所有时间为北京时间（UTC+8）。",
        "World boss schedules with name and waypoint copy. All times use Beijing time (UTC+8)."));
    if (font) ImGui::PopFont();
}

void Load(AddonAPI_t* supplied) {
    api = supplied;
    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(api->ImguiContext));
    ImGui::SetAllocatorFunctions(reinterpret_cast<void* (*)(std::size_t, void*)>(api->ImguiMalloc),
                                reinterpret_cast<void (*)(void*, void*)>(api->ImguiFree));
    nexus = static_cast<NexusLinkData_t*>(api->DataLink_Get(DL_NEXUS_LINK));
    visible.store(true);
    ReadLanguage();
    LoadChineseFont();
    LoadBossImages();
    api->GUI_Register(RT_Render, Render);
    api->GUI_Register(RT_OptionsRender, Options);
    api->InputBinds_RegisterWithString(kBind, Toggle, "CTRL+SHIFT+T");
    api->Log(LOGL_INFO, "GW2 Nexus Timer", "World boss timer v0.1.0 loaded.");
}

void Unload() {
    api->GUI_Deregister(Render);
    api->GUI_Deregister(Options);
    api->InputBinds_Deregister(kBind);
    if (font_requested) api->Fonts_Release(kFontId, ReceiveFont);
    chinese_font.store(nullptr);
    boss_texture_keys.clear();
    nexus = nullptr;
    api = nullptr;
}
} // namespace

extern "C" __declspec(dllexport) AddonDefinition_t* GetAddonDef() {
    static AddonDefinition_t definition{};
    // Private development signature; confirm uniqueness before public library submission.
    definition.Signature = static_cast<std::uint32_t>(-191016601);
    definition.APIVersion = NEXUS_API_VERSION;
    definition.Name = "GW2 World Timer";
    definition.Version = {0, 1, 0, 0};
    definition.Author = "iamapsycho";
    definition.Description = "World boss schedules and waypoint copy (CN/EN, UTC+8).";
    definition.Load = Load;
    definition.Unload = Unload;
    definition.Flags = AF_None;
#ifdef GW2_TIMER_GITHUB_URL
    definition.Provider = UP_GitHub;
    definition.UpdateLink = GW2_TIMER_GITHUB_URL;
#else
    definition.Provider = UP_None;
    definition.UpdateLink = nullptr;
#endif
    return &definition;
}
