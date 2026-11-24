#include <windows.h>
#include "Nexus.h"
#include "imgui.h"
#include "schedule.h"
#include "event_schedule.h"
#include "clipboard_win.h"
#include "boss_images.generated.h"

#include <atomic>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>

namespace {
// Use a fresh layout ID so widths saved by the previous release do not carry over.
constexpr char kWindowCN[] = u8"世界计时###GW2NexusTimerCards70";
constexpr char kWindowEN[] = "World Timer###GW2NexusTimerCards70";
constexpr float kUiScale = 0.70f;
constexpr float kUiFontSize = 18.0f;
constexpr float kDetailFontSize = 18.0f;
constexpr float kStatusFontSize = 20.0f;
constexpr char kBind[] = "GW2_NEXUS_TIMER_TOGGLE";
constexpr char kFontId[] = "GW2_NEXUS_TIMER_CHINESE_18";
constexpr char kDisplayFontId[] = "GW2_NEXUS_TIMER_DISPLAY_40";
constexpr char kDetailFontId[] = "GW2_NEXUS_TIMER_DETAILS_20";
constexpr char kMiniAppTexture[] = "GW2_TIMER_MINIAPP_CODE";
AddonAPI_t* api = nullptr;
NexusLinkData_t* nexus = nullptr;
std::atomic<ImFont*> chinese_font{nullptr};
std::atomic<ImFont*> display_font{nullptr};
std::atomic<ImFont*> detail_font{nullptr};
std::atomic<bool> visible{true};
bool font_requested = false;
ImGuiTextFilter filter;
ImGuiTextFilter event_filter;
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
    const auto& code = world_timer::kMiniAppCode;
    api->Textures_GetOrCreateFromMemory(kMiniAppTexture, const_cast<unsigned char*>(code.data), code.size);
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

void LanguageSelector(float scale = 1.0f) {
    int selected = language == world_timer::Language::CN ? 0 : 1;
    ImGui::SetNextItemWidth(110.0f * scale);
    if (!ImGui::Combo("Language###language", &selected, "CN\0EN\0")) return;
    language = selected == 0 ? world_timer::Language::CN : world_timer::Language::EN;
    if (settings_path.empty()) return;
    std::error_code error;
    std::filesystem::create_directories(settings_path.parent_path(), error);
    std::ofstream file(settings_path);
    if (file) file << "language=" << (selected == 0 ? "cn" : "en") << '\n';
    if (!file) api->Log(LOGL_WARNING, "GW2 Nexus Timer", "Could not save language setting.");
}

void DrawFilter(ImGuiTextFilter& value, const char* label) {
    const float label_width = ImGui::CalcTextSize(label, nullptr, true).x;
    value.Draw(label, std::max(100.0f, ImGui::GetContentRegionAvail().x - label_width - ImGui::GetStyle().ItemInnerSpacing.x));
}

void MutedWrapped(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.73f, 0.77f, 1));
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
}

std::int64_t Now() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void ReceiveFont(const char* identifier, void* font) {
    if (identifier && std::string(identifier) == kDisplayFontId)
        display_font.store(static_cast<ImFont*>(font));
    else if (identifier && std::string(identifier) == kDetailFontId)
        detail_font.store(static_cast<ImFont*>(font));
    else
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
        u8"显示隐藏可在中修改时刻数据以本机系统时钟计算所有时间为今天明天昨天·（）"
        u8"上次下次本次计划进行中即将开始尚未开始计划开始后已过倒计时刷新后分钟展示窗口非实例状态点击图片或名称查看全天时刻点击地图复制暂无匹配结果关闭"
        u8"世界事件时间轴分类全部一条龙范围小时现在等待计划阶段当前非实时以游戏内为准点击色块复制名称传送点推测时段不保证刷新页来源星岬岛切换距离下次计划开始的倒计时";
    glyphs += u8"《》：；…关于作者激战助手面向国服玩家的随身提供日常周常清单世界与地图事件计时物品资料查询职业配装传奇合成和图文攻略方便在手机上随时查阅微信扫一扫打开小程序点击放大扫码入口加载中关闭版本本插件用于游戏内查看计划时刻";
    for (const auto& boss : world_timer::Bosses()) glyphs += boss.name + boss.map + boss.zone;
    for (const auto& event : world_timer::Events()) {
        glyphs += event.name + event.category;
        for (const auto& phase : event.phases) glyphs += phase.name;
    }
    for (const auto& group : world_timer::EventGroups()) glyphs += group.name;
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
        api->Fonts_AddFromFile(kFontId, kUiFontSize, path.u8string().c_str(), ReceiveFont, &config);
        api->Fonts_AddFromFile(kDisplayFontId, 40.0f, path.u8string().c_str(), ReceiveFont, &config);
        api->Fonts_AddFromFile(kDetailFontId, kStatusFontSize, path.u8string().c_str(), ReceiveFont, &config);
        font_requested = true;
        break;
    }
}

void CopyOccurrence(const world_timer::Occurrence& occurrence, std::int64_t now) {
    feedback = CopyUnicodeText(world_timer::CopyText(occurrence, now, language)) ? 1 : 2;
    feedback_until = std::chrono::steady_clock::now() + std::chrono::seconds(4);
}

std::string RelativeClock(std::int64_t time, std::int64_t now) {
    const auto day = world_timer::BeijingDayStart(time);
    const auto today = world_timer::BeijingDayStart(now);
    const char* prefix = day < today ? T(u8"昨天 ", "Yesterday ")
                       : day > today ? T(u8"明天 ", "Tomorrow ") : "";
    return prefix + world_timer::ClockText(time);
}

void DrawBossCard(const world_timer::CardOccurrence& row, std::int64_t now, float width, float scale) {
    const auto& boss = *row.next.boss;
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float height = std::max(184.0f, 212.0f * scale);
    const float picture_width = 202.0f * scale;
    const float rounding = 10.0f * scale;
    const ImVec2 end(origin.x + width, origin.y + height);
    const float text_x = origin.x + picture_width + 24.0f * scale;
    const float text_width = end.x - text_x - 18.0f * scale;
    const bool soon = !row.recent && row.next.starts_at - now <= 900;
    auto* draw = ImGui::GetWindowDrawList();
    auto* card_font = display_font.load();
    if (!card_font) card_font = ImGui::GetFont();
    auto* details = detail_font.load();
    if (!details) details = ImGui::GetFont();
    // Keep secondary text readable even when the compact layout narrows a card.
    const float detail_size = kDetailFontSize;
    const float status_size = kStatusFontSize;
    ImGui::PushID(boss.id.c_str());
    ImGui::Dummy(ImVec2(width, height));
    // Keep all hit targets within this one layout item, including in a scrolled child.
    const auto after = ImGui::GetCursorScreenPos();
    if (!ImGui::IsRectVisible(origin, end)) {
        ImGui::PopID();
        return;
    }
    draw->AddRectFilled(ImVec2(origin.x + 2 * scale, origin.y + 4 * scale), ImVec2(end.x + 2 * scale, end.y + 4 * scale),
                        IM_COL32(0, 0, 0, 40), rounding);
    draw->AddRectFilled(origin, end, IM_COL32(47, 47, 47, 255), rounding);
    const auto texture_key = boss_texture_keys.find(boss.id);
    auto* picture = texture_key != boss_texture_keys.end() ? api->Textures_Get(texture_key->second.c_str()) : nullptr;
    if (picture && picture->Resource && picture->Width && picture->Height) {
        // Cover the full image panel with a centered crop, preserving its aspect ratio.
        const float source_aspect = static_cast<float>(picture->Width) / picture->Height;
        const float target_aspect = picture_width / height;
        ImVec2 uv0(0, 0), uv1(1, 1);
        if (source_aspect > target_aspect) {
            uv0.x = (1.0f - target_aspect / source_aspect) * 0.5f;
            uv1.x = 1.0f - uv0.x;
        } else {
            uv0.y = (1.0f - source_aspect / target_aspect) * 0.5f;
            uv1.y = 1.0f - uv0.y;
        }
        draw->AddImageRounded(picture->Resource, origin, ImVec2(origin.x + picture_width, end.y),
                              uv0, uv1, IM_COL32_WHITE, rounding, ImDrawCornerFlags_Left);
    } else {
        draw->AddRectFilled(origin, ImVec2(origin.x + picture_width, end.y),
                            IM_COL32(34, 39, 40, 255), rounding, ImDrawCornerFlags_Left);
    }

    const auto& name = world_timer::BossName(boss, language);
    const float desired_name_size = std::max(28.0f, 44.0f * scale);
    const float natural_name_width = card_font->CalcTextSizeA(desired_name_size, FLT_MAX, 0, name.c_str()).x;
    const float name_size = std::max(22.0f, desired_name_size * std::min(1.0f, text_width / std::max(1.0f, natural_name_width)));
    const ImVec4 clip(text_x, origin.y, end.x - 12.0f * scale, end.y);
    draw->AddText(card_font, name_size, ImVec2(text_x, origin.y + 10.0f),
                  IM_COL32_WHITE, name.c_str(), nullptr, 0, &clip);
    const auto displayed = row.recent ? row.previous : row.next;
    const auto preceding = row.recent ? world_timer::PreviousOccurrence(boss, row.previous.starts_at - 1) : row.previous;
    const auto times = std::string(T(u8"上次: ", "Last: ")) + RelativeClock(preceding.starts_at, now)
        + "   " + (row.recent ? T(u8"本次: ", "This: ") : T(u8"下次: ", "Next: "))
        + RelativeClock(displayed.starts_at, now);
    draw->AddText(details, detail_size, ImVec2(text_x, origin.y + 48.0f),
                  IM_COL32(230, 231, 233, 255), times.c_str(), nullptr, 0, &clip);
    ImGui::SetCursorScreenPos(ImVec2(text_x, origin.y + 48.0f));
    ImGui::InvisibleButton("times", ImVec2(text_width, 22.0f));
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", times.c_str());

    ImGui::SetCursorScreenPos(origin);
    if (ImGui::InvisibleButton("picture", ImVec2(picture_width, height))) ImGui::OpenPopup("daily");
    ImGui::SetCursorScreenPos(ImVec2(text_x, origin.y + 10.0f));
    if (ImGui::InvisibleButton("name", ImVec2(text_width, 36.0f))) ImGui::OpenPopup("daily");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s\n%s", name.c_str(),
        T(u8"点击名称查看全天时刻", "Click for the daily schedule"));

    const auto location = world_timer::MapName(boss, language) + " " + boss.waypoint;
    ImGui::SetCursorScreenPos(ImVec2(text_x, origin.y + 72.0f));
    if (ImGui::InvisibleButton("copy", ImVec2(text_width, 22.0f)))
        CopyOccurrence(row.recent ? row.previous : row.next, now);
    const bool copy_hover = ImGui::IsItemHovered();
    draw->AddText(details, detail_size, ImVec2(text_x, origin.y + 72.0f),
                  copy_hover ? IM_COL32(255, 206, 98, 255) : IM_COL32(240, 240, 240, 255),
                  location.c_str(), nullptr, 0, &clip);
    if (copy_hover) ImGui::SetTooltip("%s\n%s", location.c_str(), T(u8"复制名称与传送点", "Copy name & waypoint"));

    const auto timer = (row.recent ? "+" : "") + world_timer::Countdown(row.recent
        ? now - row.previous.starts_at : row.next.starts_at - now);
    const char* status = row.recent ? T(u8"计划进行中", "Scheduled")
        : soon ? T(u8"即将开始", "Soon") : T(u8"尚未开始", "Upcoming");
    const auto status_color = row.recent ? IM_COL32(190, 58, 40, 255)
        : soon ? IM_COL32(255, 185, 26, 255) : IM_COL32(88, 99, 115, 255);
    const auto status_text = soon ? IM_COL32(30, 30, 30, 255) : IM_COL32_WHITE;
    const ImVec2 status_pos(text_x, origin.y + 98.0f);
    const auto status_width = details->CalcTextSizeA(status_size, FLT_MAX, 0, status).x;
    draw->AddRectFilled(status_pos, ImVec2(status_pos.x + status_width + 12, status_pos.y + status_size + 4),
                        status_color, 3);
    draw->AddText(details, status_size, ImVec2(status_pos.x + 6, status_pos.y + 1), status_text, status);
    ImGui::SetCursorScreenPos(status_pos);
    ImGui::InvisibleButton("status", ImVec2(status_width + 12, status_size + 4));
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s\n%s", status,
        row.recent ? T(u8"计划开始后已过，非实例实时状态", "Time since scheduled start; not live instance status")
                   : T(u8"距离下次计划开始的倒计时", "Countdown to the next scheduled start"));
    const float desired_timer_size = std::max(38.0f, 56.0f * scale);
    const float natural_timer_width = card_font->CalcTextSizeA(desired_timer_size, FLT_MAX, 0, timer.c_str()).x;
    const float timer_size = desired_timer_size * std::min(1.0f, text_width / std::max(1.0f, natural_timer_width));
    draw->AddText(card_font, timer_size, ImVec2(text_x, origin.y + 132.0f),
                  IM_COL32_WHITE, timer.c_str(), nullptr, 0, &clip);
    if (ImGui::BeginPopup("daily")) {
        ImGui::TextUnformatted(name.c_str());
        ImGui::TextDisabled("%s", T(u8"全天时刻（北京时间）", "Daily schedule (UTC+8)"));
        ImGui::Separator();
        for (std::size_t i = 0; i < boss.minutes.size(); ++i) {
            if (i % 4 != 0) ImGui::SameLine(0, 16.0f * kUiScale);
            ImGui::Text("%02d:%02d", boss.minutes[i] / 60, boss.minutes[i] % 60);
        }
        ImGui::Separator();
        if (ImGui::Button(T(u8"复制名称与传送点", "Copy name & waypoint")))
            CopyOccurrence(row.recent ? row.previous : row.next, now);
        ImGui::EndPopup();
    }
    ImGui::SetCursorScreenPos(after);
    ImGui::PopID();
}

void RenderEventTimeline(std::int64_t now) {
    static int category = 0;
    static int span_index = 0;
    static std::int64_t offset_minutes = 0;
    const auto& groups = world_timer::EventGroups();
    const ImVec4 accents[] = {
        {0.70f, 0.74f, 0.80f, 1}, {0.46f, 0.51f, 1, 1}, {0.46f, 0.51f, 1, 1},
        {0.15f, 0.70f, 1, 1}, {0.15f, 0.70f, 1, 1}, {1, 0.70f, 0.20f, 1},
        {0.18f, 0.83f, 0.50f, 1}, {1, 0.30f, 0.43f, 1}, {0.15f, 0.70f, 1, 1},
        {0.74f, 0.44f, 0.95f, 1}, {0.34f, 0.49f, 0.96f, 1},
    };
    bool category_changed = false;
    const float right = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, kUiScale);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4 * kUiScale);
    for (std::size_t i = 0; i < groups.size(); ++i) {
        const auto& group = groups[i];
        const auto& label = language == world_timer::Language::CN ? group.name : group.name_en;
        const float width = ImGui::CalcTextSize(label.c_str()).x + 18 * kUiScale;
        if (i && ImGui::GetItemRectMax().x + 5 * kUiScale + width <= right) ImGui::SameLine(0, 5 * kUiScale);
        const auto accent = accents[i];
        const bool selected = category == static_cast<int>(i);
        ImGui::PushStyleColor(ImGuiCol_Border, accent);
        ImGui::PushStyleColor(ImGuiCol_Text, selected ? ImVec4(1, 1, 1, 1) : accent);
        ImGui::PushStyleColor(ImGuiCol_Button, selected ? ImVec4(accent.x * 0.65f, accent.y * 0.65f, accent.z * 0.65f, 1) : ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(accent.x * 0.45f, accent.y * 0.45f, accent.z * 0.45f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(accent.x * 0.65f, accent.y * 0.65f, accent.z * 0.65f, 1));
        ImGui::PushID(group.id.c_str());
        if (ImGui::Button(label.c_str(), ImVec2(width, ImGui::GetFrameHeight() + 2))) {
            category_changed = category != static_cast<int>(i);
            category = static_cast<int>(i);
        }
        ImGui::PopID();
        ImGui::PopStyleColor(5);
    }
    ImGui::PopStyleVar(2);
    MutedWrapped(T(u8"点击色块复制事件与传送点 · 固定计划阶段，以游戏内为准",
        "Click a phase to copy | Scheduled phases; check actual status in game"));
    ImGui::SetNextItemWidth(110.0f);
    ImGui::Combo(T(u8"范围###EventRange", "Range###EventRange"), &span_index,
                 T(u8"2小时\0" u8"4小时\0" u8"24小时\0", "2 hours\0" "4 hours\0" "24 hours\0"));
    const int span_minutes = span_index == 0 ? 120 : span_index == 1 ? 240 : 1440;
    ImGui::SameLine();
    if (ImGui::Button("<###PrevEvents")) offset_minutes -= span_minutes;
    ImGui::SameLine();
    if (ImGui::Button(T(u8"现在###NowEvents", "Now###NowEvents"))) offset_minutes = 0;
    ImGui::SameLine();
    if (ImGui::Button(">###NextEvents")) offset_minutes += span_minutes;
    DrawFilter(event_filter, T(u8"筛选名称 / 地图###EventFilter", "Name / map###EventFilter"));

    const auto day = world_timer::BeijingDayStart(now);
    const auto begin = day + ((now - day) / 1800) * 1800 - 900 + offset_minutes * 60;
    const auto finish = begin + span_minutes * 60;
    static std::int64_t cached_begin = 0, cached_finish = 0;
    static std::vector<std::vector<world_timer::EventWindow>> cached;
    if (cached.empty() || cached_begin != begin || cached_finish != finish) {
        cached.clear();
        for (const auto& row : world_timer::Events()) cached.push_back(world_timer::EventWindows(row, begin, finish));
        cached_begin = begin;
        cached_finish = finish;
    }
    const float name_width = 152.0f;
    const auto axis = ImGui::GetCursorScreenPos();
    const float axis_width = ImGui::GetContentRegionAvail().x - name_width - ImGui::GetStyle().ScrollbarSize;
    auto* axis_draw = ImGui::GetWindowDrawList();
    axis_draw->AddText(axis, IM_COL32(180, 190, 200, 255), T(u8"地图 / 事件", "Map / event"));
    const int tick_minutes = std::max(span_minutes <= 240 ? 30 : 120,
        static_cast<int>(std::ceil(span_minutes * 76.0f / std::max(1.0f, axis_width) / 30.0f)) * 30);
    const auto tick_seconds = tick_minutes * 60;
    for (auto tick = ((begin + tick_seconds - 1) / tick_seconds) * tick_seconds; tick < finish; tick += tick_seconds) {
        const float x = axis.x + name_width + axis_width * static_cast<float>(tick - begin) / (span_minutes * 60);
        const auto label = world_timer::ClockText(tick);
        if (x + ImGui::CalcTextSize(label.c_str()).x <= axis.x + name_width + axis_width)
            axis_draw->AddText(ImVec2(x, axis.y), IM_COL32(205, 211, 220, 255), label.c_str());
    }
    ImGui::Dummy(ImVec2(0, ImGui::GetTextLineHeight() + 4));
    const float footer = ImGui::GetTextLineHeightWithSpacing() * 4.0f;
    ImGui::BeginChild("event_timeline", ImVec2(0, -footer), false);
    if (category_changed) ImGui::SetScrollY(0);
    int count = 0;
    for (std::size_t index = 0; index < world_timer::Events().size(); ++index) {
        const auto& row = world_timer::Events()[index];
        if (!world_timer::EventInGroup(row, groups[category])) continue;
        std::string search = row.name + " " + row.name_en + " " + row.category + " " + row.category_en;
        for (const auto& phase : row.phases) search += " " + phase.name + " " + phase.name_en;
        if (!event_filter.PassFilter(search.c_str())) continue;
        ++count;
        ImGui::PushID(row.id.c_str());
        const auto origin = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        const float height = 56.0f;
        const ImVec2 end(origin.x + width, origin.y + height);
        ImGui::Dummy(ImVec2(width, height));
        const auto after = ImGui::GetCursorScreenPos();
        if (!ImGui::IsRectVisible(origin, end)) { ImGui::PopID(); continue; }
        auto* draw = ImGui::GetWindowDrawList();
        const auto& title = world_timer::EventName(row, language);
        const float font_size = ImGui::GetFontSize();
        const ImVec4 name_clip(origin.x, origin.y, origin.x + name_width - 4 * kUiScale, end.y);
        const float title_height = ImGui::GetFont()->CalcTextSizeA(font_size, FLT_MAX, name_width - 8 * kUiScale, title.c_str()).y;
        draw->AddText(ImGui::GetFont(), font_size, ImVec2(origin.x, origin.y + 3 * kUiScale),
                      IM_COL32(235, 238, 242, 255), title.c_str(), nullptr, name_width - 8 * kUiScale, &name_clip);
        if (title_height < font_size * 1.5f)
            draw->AddText(ImGui::GetFont(), font_size, ImVec2(origin.x, origin.y + 28),
                          IM_COL32(180, 187, 196, 255), world_timer::EventCategory(row, language).c_str(), nullptr, 0, &name_clip);
        ImGui::SetCursorScreenPos(origin);
        ImGui::InvisibleButton("event_name", ImVec2(name_width - 4 * kUiScale, height));
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s\n%s", title.c_str(), world_timer::EventCategory(row, language).c_str());
        const float timeline_width = width - name_width;
        for (const auto& window : cached[index]) {
            const auto& phase = *window.phase;
            const float x0 = origin.x + name_width + timeline_width * static_cast<float>(std::max(begin, window.starts_at) - begin) / (span_minutes * 60);
            const float x1 = origin.x + name_width + timeline_width * static_cast<float>(std::min(finish, window.ends_at) - begin) / (span_minutes * 60);
            if (x1 - x0 <= kUiScale) continue;
            const ImVec2 block(x0, origin.y + 2 * kUiScale), block_end(x1 - kUiScale, end.y - 2 * kUiScale);
            const auto color = phase.active ? IM_COL32(phase.color[0], phase.color[1], phase.color[2], 255) : IM_COL32(40, 47, 56, 255);
            draw->AddRectFilled(block, block_end, color, 3 * kUiScale);
            const bool current = window.starts_at <= now && now < window.ends_at;
            if (current && phase.active) draw->AddRect(block, block_end, IM_COL32(255, 213, 105, 255), 3 * kUiScale, ImDrawCornerFlags_All, kUiScale);
            if (x1 - x0 > 14 * kUiScale) {
                const ImVec4 clip(x0 + 3 * kUiScale, block.y, x1 - 3 * kUiScale, block_end.y);
                const auto text_color = phase.active ? IM_COL32(28, 34, 40, 255) : IM_COL32(150, 159, 170, 255);
                draw->AddText(ImGui::GetFont(), font_size, ImVec2(x0 + 4 * kUiScale, origin.y + 6 * kUiScale),
                              text_color, world_timer::PhaseName(phase, language).c_str(), nullptr, 0, &clip);
                const auto time = world_timer::ClockText(window.starts_at);
                draw->AddText(ImGui::GetFont(), font_size, ImVec2(x0 + 4 * kUiScale, origin.y + 29),
                              text_color, time.c_str(), nullptr, 0, &clip);
            }
            ImGui::SetCursorScreenPos(block);
            ImGui::PushID(std::to_string(window.starts_at).c_str());
            if (ImGui::InvisibleButton("phase", ImVec2(x1 - x0, height - 4 * kUiScale)) && phase.active) {
                feedback = CopyUnicodeText(world_timer::EventCopyText(window, now, language)) ? 1 : 2;
                feedback_until = std::chrono::steady_clock::now() + std::chrono::seconds(4);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("%s - %s", title.c_str(), world_timer::PhaseName(phase, language).c_str());
                ImGui::Text("%s - %s (UTC+8)", world_timer::EventTimeText(window.starts_at, now, language).c_str(),
                            world_timer::EventTimeText(window.ends_at, now, language).c_str());
                if (current && phase.active) ImGui::TextColored(ImVec4(1, 0.76f, 0.35f, 1), "%s",
                    T(u8"当前计划阶段，非实例实时状态", "Current scheduled phase, not live instance status"));
                if (row.approximate) ImGui::TextColored(ImVec4(1, 0.65f, 0.4f, 1), "%s",
                    T(u8"推测时段，不保证刷新", "Approximate window, not guaranteed"));
                if (phase.active) ImGui::TextDisabled("%s", T(u8"点击色块复制事件与传送点", "Click to copy event & waypoints"));
                ImGui::EndTooltip();
            }
            ImGui::PopID();
        }
        if (now >= begin && now < finish) {
            const float x = origin.x + name_width + timeline_width * static_cast<float>(now - begin) / (span_minutes * 60);
            draw->AddLine(ImVec2(x, origin.y), ImVec2(x, end.y), IM_COL32(255, 96, 78, 255), 1.5f * kUiScale);
        }
        ImGui::SetCursorScreenPos(after);
        ImGui::PopID();
    }
    if (!count) ImGui::TextDisabled("%s", T(u8"暂无匹配结果", "No matching events"));
    ImGui::EndChild();
    const auto summary = world_timer::EventTimeText(begin, now, language) + " - "
        + world_timer::EventTimeText(finish, now, language) + " (UTC+8)";
    MutedWrapped(summary.c_str());
}

int PushTimerStyle() {
    const auto style = ImGui::GetStyle();
    const std::pair<ImGuiStyleVar, ImVec2> vectors[] = {
        {ImGuiStyleVar_WindowPadding, ImVec2(16, 12)},
        {ImGuiStyleVar_ItemSpacing, ImVec2(10, 8)},
        {ImGuiStyleVar_FramePadding, style.FramePadding},
        {ImGuiStyleVar_ItemInnerSpacing, style.ItemInnerSpacing},
        {ImGuiStyleVar_CellPadding, style.CellPadding},
        {ImGuiStyleVar_WindowMinSize, style.WindowMinSize},
    };
    const std::pair<ImGuiStyleVar, float> scalars[] = {
        {ImGuiStyleVar_WindowRounding, 10.0f},
        {ImGuiStyleVar_WindowBorderSize, style.WindowBorderSize},
        {ImGuiStyleVar_ChildRounding, style.ChildRounding},
        {ImGuiStyleVar_ChildBorderSize, style.ChildBorderSize},
        {ImGuiStyleVar_PopupRounding, style.PopupRounding},
        {ImGuiStyleVar_PopupBorderSize, style.PopupBorderSize},
        {ImGuiStyleVar_FrameRounding, style.FrameRounding},
        {ImGuiStyleVar_FrameBorderSize, style.FrameBorderSize},
        {ImGuiStyleVar_IndentSpacing, style.IndentSpacing},
        {ImGuiStyleVar_ScrollbarSize, style.ScrollbarSize},
        {ImGuiStyleVar_ScrollbarRounding, style.ScrollbarRounding},
        {ImGuiStyleVar_GrabMinSize, style.GrabMinSize},
        {ImGuiStyleVar_GrabRounding, style.GrabRounding},
        {ImGuiStyleVar_TabRounding, style.TabRounding},
    };
    int count = 0;
    for (const auto& value : vectors) {
        ImGui::PushStyleVar(value.first, ImVec2(value.second.x * kUiScale, value.second.y * kUiScale));
        ++count;
    }
    for (const auto& value : scalars) {
        ImGui::PushStyleVar(value.first, value.second * kUiScale);
        ++count;
    }
    return count;
}

void RenderBossCards(std::int64_t now, bool font_available) {
    MutedWrapped(T(u8"点击图片或名称查看全天时刻 · 点击地图复制名称与传送点",
        "Click image / name for the daily schedule | Click map to copy name & waypoint"));
    if (!font_available) {
        ImGui::TextWrapped("Chinese font unavailable/loading. Requires Microsoft YaHei or SimHei in Windows Fonts.");
    }
    DrawFilter(filter, T(u8"筛选名称 / 地图###BossFilter", "Name / map###BossFilter"));

    // Rebuild at most once a second, rather than calculating schedules every frame.
    static std::int64_t cached_second = -1;
    static std::vector<world_timer::CardOccurrence> upcoming;
    if (cached_second != now) {
        upcoming = world_timer::CardOccurrences(world_timer::Bosses(), now);
        cached_second = now;
    }
    const float footer = ImGui::GetTextLineHeightWithSpacing() * 4.0f;
    ImGui::BeginChild("bosses", ImVec2(0, -footer), false);
    const float available = ImGui::GetContentRegionAvail().x;
    const int columns = available >= 1000.0f * kUiScale ? 2 : 1;
    const float gap = 16.0f * kUiScale;
    const float card_width = (available - gap * (columns - 1)) / columns;
    const float scale = std::min(1.15f * kUiScale, card_width / 540.0f);
    int count = 0;
    if (ImGui::BeginTable("cards", columns, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoPadOuterX)) {
        for (const auto& row : upcoming) {
            const auto& boss = *row.next.boss;
            const auto search = boss.name + " " + boss.map + " " + boss.zone
                + " " + boss.name_en + " " + boss.map_en + " " + boss.zone_en;
            if (!filter.PassFilter(search.c_str())) continue;
            ImGui::TableNextColumn();
            DrawBossCard(row, now, ImGui::GetContentRegionAvail().x, scale);
            ++count;
        }
        ImGui::EndTable();
    }
    if (!count) ImGui::TextDisabled("%s", T(u8"暂无匹配结果", "No matching bosses / maps"));
    ImGui::EndChild();
    MutedWrapped(T(u8"固定计划 · 进行中为开始后15分钟，非实例状态",
        "Fixed schedule | Scheduled: first 15 min, not live status"));
}

void MiniAppCode(float side) {
    auto* code = api->Textures_Get(kMiniAppTexture);
    if (!code || !code->Resource) {
        MutedWrapped(T(u8"小程序码加载中…", "Loading mini-program code..."));
        return;
    }
    // Preserve the complete square code and its white border; never crop it.
    if (ImGui::ImageButton(code->Resource, ImVec2(side, side), ImVec2(0, 0), ImVec2(1, 1), 10,
                           ImVec4(1, 1, 1, 1))) ImGui::OpenPopup("miniapp_code");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", T(u8"微信扫一扫打开小程序 · 点击放大", "Scan in WeChat to open | Click to enlarge"));
    if (ImGui::BeginPopup("miniapp_code", ImGuiWindowFlags_AlwaysAutoResize)) {
        const auto screen = ImGui::GetIO().DisplaySize;
        const float enlarged = std::max(128.0f, std::min({512.0f, screen.x - 72.0f, screen.y - 120.0f}));
        ImGui::TextUnformatted(T(u8"微信扫一扫打开小程序", "Scan with WeChat to open"));
        ImGui::Image(code->Resource, ImVec2(enlarged, enlarged));
        if (ImGui::Button(T(u8"关闭", "Close"))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void MiniAppIntroduction() {
    ImGui::TextUnformatted(T(u8"激战2助手 · 微信小程序", "GW2 Companion | WeChat mini program"));
    ImGui::Spacing();
    ImGui::TextWrapped("%s", T(
        u8"面向《激战2》国服玩家的随身助手，提供日常/周常清单、世界BOSS与地图事件计时、物品资料查询、职业配装、传奇合成和图文攻略，方便在手机上随时查阅。",
        "A pocket companion for Guild Wars 2 players on the Chinese servers: daily and weekly checklists, boss and map event timers, item lookup, profession builds, legendary crafting and illustrated guides."));
    ImGui::Spacing();
    MutedWrapped(T(u8"用手机微信扫一扫小程序码即可进入；点击小程序码可放大。",
        "Scan the code with WeChat on your phone to open the mini program. Click the code to enlarge it."));
}

void RenderAbout() {
    ImGui::BeginChild("about_author", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing() * 3), false);
    ImGui::TextUnformatted(T(u8"作者：iamapsycho", "Author: iamapsycho"));
    MutedWrapped(T(u8"世界计时 v0.1.5 · Nexus 插件", "World Timer v0.1.5 | Nexus addon"));
    ImGui::Separator();
    const float available = ImGui::GetContentRegionAvail().x;
    if (available >= 600.0f && ImGui::BeginTable("about_columns", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("code", ImGuiTableColumnFlags_WidthFixed, 292.0f);
        ImGui::TableSetupColumn("intro", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextColumn();
        MiniAppCode(268.0f);
        ImGui::TableNextColumn();
        MiniAppIntroduction();
        ImGui::EndTable();
    } else {
        MiniAppCode(std::min(268.0f, available - 24.0f));
        MiniAppIntroduction();
    }
    ImGui::EndChild();
}

void RenderAt(std::int64_t now) {
    if (!visible.load() || (nexus && !nexus->IsGameplay)) return;
    ImFont* font = chinese_font.load();
    if (font) ImGui::PushFont(font);
    bool open = visible.load();
    const int style_count = PushTimerStyle();
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.075f, 0.085f, 0.10f, 0.98f));
    ImGui::SetNextWindowSize(ImVec2(1140 * kUiScale, 850 * kUiScale), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(520 * kUiScale, 440 * kUiScale), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::Begin(language == world_timer::Language::CN ? kWindowCN : kWindowEN, &open)) {
        LanguageSelector(kUiScale);
        const auto heading = std::string(T(u8"北京时间 ", "Beijing (UTC+8) ")) + world_timer::ClockText(now)
            + " | " + std::to_string(world_timer::Bosses().size()) + T(u8" 项首领 / ", " bosses / ")
            + std::to_string(world_timer::Events().size()) + T(u8" 条时间轴", " timelines");
        if (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + ImGui::CalcTextSize(heading.c_str()).x
            <= ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x) ImGui::SameLine();
        ImGui::TextWrapped("%s", heading.c_str());
        bool events_tab = false;
        bool about_tab = false;
        if (ImGui::BeginTabBar("timer_tabs")) {
            if (ImGui::BeginTabItem(T(u8"世界BOSS###BossTab", "World bosses###BossTab"))) {
                RenderBossCards(now, font != nullptr);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(T(u8"事件时间轴###EventTab", "Event timeline###EventTab"))) {
                events_tab = true;
                RenderEventTimeline(now);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(T(u8"关于作者###AboutTab", "About the author###AboutTab"))) {
                about_tab = true;
                RenderAbout();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        if (std::chrono::steady_clock::now() < feedback_until)
            ImGui::TextWrapped("%s", feedback == 1
                ? T(u8"已复制名称与传送点，可粘贴到游戏聊天框", "Copied. Paste into game chat.")
                : T(u8"复制失败，剪贴板可能被占用，请重试", "Clipboard unavailable. Please try again."));
        else
            ImGui::TextDisabled(" ");
        MutedWrapped(T(u8"Ctrl+Shift+T 显示 / 隐藏（可在 Nexus 中修改）",
            "Ctrl+Shift+T to toggle (configurable in Nexus)"));
        if (about_tab)
            MutedWrapped(T(u8"微信扫一扫 · 小程序入口", "Scan in WeChat | Mini-program entry"));
        else if (events_tab)
            MutedWrapped((std::string(T(u8"数据 ", "Data ")) + world_timer::EventSnapshotDate()
                + T(u8" · 星岬岛 · 本机系统时钟", " | WishingStar | System clock")).c_str());
        else
            MutedWrapped((std::string(T(u8"数据 ", "Data ")) + world_timer::SnapshotDate()
                + T(u8" · 本机系统时钟", " | System clock")).c_str());
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(style_count);
    if (!open) visible.store(false);
    if (font) ImGui::PopFont();
}

void Render() { RenderAt(Now()); }

void Options() {
    ImFont* font = chinese_font.load();
    if (font) ImGui::PushFont(font);
    ImGui::TextUnformatted(T(u8"世界计时 v0.1.5", "World Timer v0.1.5"));
    LanguageSelector();
    bool show = visible.load();
    if (ImGui::Checkbox(T(u8"显示世界计时###show", "Show world timer###show"), &show)) visible.store(show);
    ImGui::TextWrapped("%s", T(u8"查看世界BOSS与事件时间轴，复制名称与传送点。所有时间为北京时间（UTC+8）。",
        "World boss schedules and event timelines with name and waypoint copy. All times use Beijing time (UTC+8)."));
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
    api->Log(LOGL_INFO, "GW2 Nexus Timer", "World timer v0.1.5 loaded.");
}

void Unload() {
    api->GUI_Deregister(Render);
    api->GUI_Deregister(Options);
    api->InputBinds_Deregister(kBind);
    if (font_requested) {
        api->Fonts_Release(kFontId, ReceiveFont);
        api->Fonts_Release(kDisplayFontId, ReceiveFont);
        api->Fonts_Release(kDetailFontId, ReceiveFont);
    }
    chinese_font.store(nullptr);
    display_font.store(nullptr);
    detail_font.store(nullptr);
    font_requested = false;
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
    definition.Version = {0, 1, 5, 0};
    definition.Author = "iamapsycho";
    definition.Description = "World boss cards, event timelines, waypoint copy and author / WeChat mini program (CN/EN, UTC+8).";
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
