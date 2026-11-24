#include "event_schedule.h"
#include "event_data.generated.h"
#include <algorithm>
#include <ctime>
#include <stdexcept>

namespace world_timer {
namespace {
std::int64_t FloorDiv(std::int64_t value, std::int64_t divisor) {
    return value / divisor - (value % divisor < 0 ? 1 : 0);
}
int PositiveMod(std::int64_t value, int divisor) {
    return static_cast<int>((value % divisor + divisor) % divisor);
}
const EventPhase& Phase(const EventRow& event, int id) {
    const auto phase = std::find_if(event.phases.begin(), event.phases.end(),
                                   [id](const auto& value) { return value.id == id; });
    if (phase == event.phases.end()) throw std::invalid_argument("Unknown event phase");
    return *phase;
}
} // namespace

const std::vector<EventRow>& Events() { return event_generated::kEvents; }
const std::vector<EventGroup>& EventGroups() {
    // Match the source page's shortcuts, including overlapping special/meta rows.
    static const std::vector<EventGroup> groups = {
        {"all", u8"所有", "All", {}},
        {"voe", u8"永恒愿景", "VoE", {"shs", "stw", "voe_eg", "depths_cruelty", "voe_noe"}},
        {"jw", u8"珍瑟", "Janthir", {"jwjs", "jwcmb", "jwbn"}},
        {"soo", u8"天界之谜", "SotO", {"soometa", "sotottv"}},
        {"ca", u8"凯珊", "Cantha", {"cadaynight", "casp", "cankc", "catew", "cade"}},
        {"core", u8"泰瑞亚", "Tyria", {"daynight", "wb", "hwb", "la", "pvpat", "DryTop", "ClockworkChaos"}},
        {"hot", u8"迈古玛", "Maguuma", {"VerdantBrink", "AuricBasin", "TangledDepths", "DragonStand", "LakeDoric"}},
        {"pof", u8"烈焰征途", "PoF", {"DomainOfIstan", "JahaiBluffs", "AncestorTree", "ThunderheadPeaks",
            "CrystalOasis", "DesertHighlands", "ElonRiverlands", "TheDesolation", "DomainOfVabbi"}},
        {"ice", u8"冰巢传说", "Icebrood", {"GrothmarValley", "BjoraMarches", "EyeOfTheNorth"}},
        {"special", u8"特殊事件", "Special", {"la", "JoKoAttack", "BloodstoneCrazed", "Zeghai", "ClockworkChaos"}},
        {"one", u8"一条龙", "Meta train", {"VerdantBrink", "la", "TangledDepths", "AuricBasin", "BjoraMarches",
            "hwb", "CrystalOasis", "TheDesolation", "DomainOfVabbi", "ElonRiverlands", "ThunderheadPeaks"}},
    };
    return groups;
}
bool EventInGroup(const EventRow& event, const EventGroup& group) {
    return group.id == "all" || std::find(group.row_ids.begin(), group.row_ids.end(), event.id) != group.row_ids.end();
}
const char* EventSnapshotDate() { return event_generated::kSnapshotDate; }
const std::string& EventName(const EventRow& event, Language language) {
    return language == Language::EN ? event.name_en : event.name;
}
const std::string& EventCategory(const EventRow& event, Language language) {
    return language == Language::EN ? event.category_en : event.category;
}
const std::string& PhaseName(const EventPhase& phase, Language language) {
    return language == Language::EN ? phase.name_en : phase.name;
}

std::vector<EventWindow> EventWindows(const EventRow& event, std::int64_t begin, std::int64_t end) {
    if (end <= begin || end - begin > 7 * kDaySeconds)
        throw std::invalid_argument("Event window must span between zero and seven days");
    std::vector<EventWindow> result;
    bool previous_clipped = false;
    // Include the preceding UTC day so a phase continuing through 08:00 Beijing
    // retains its real scheduled start, rather than restarting at the day boundary.
    for (auto day_index = FloorDiv(begin, kDaySeconds) - 1;
         day_index <= FloorDiv(end - 1, kDaySeconds); ++day_index) {
        const auto day = day_index * kDaySeconds;
        std::vector<PhaseStep> rotation;
        const std::vector<PhaseStep>* partial = &event.partial;
        if (!event.weekly_partials.empty()) {
            if (event.weekly_partials.size() != 7) throw std::invalid_argument("Expected seven weekday schedules");
            partial = &event.weekly_partials[PositiveMod(day_index + 3, 7)];
        }
        if (event.zeghai_rotation) {
            // Source rotation anchor: 2018-05-10 00:00 UTC; each entry lasts 25 min,
            // followed by 35 min idle. The first entry each UTC day begins at :15.
            constexpr std::int64_t anchor = 1525910400;
            const auto hours = (day - anchor) / 3600;
            rotation.push_back({0, 15});
            for (int i = 1; i <= 24; ++i) {
                rotation.push_back({PositiveMod(hours + i, 15) + 1, 25});
                rotation.push_back({0, 35});
            }
            partial = &rotation;
        }
        int minute = 0;
        auto append = [&](const PhaseStep& step) {
            if (step.minutes <= 0) throw std::invalid_argument("Invalid event duration");
            const auto& phase = Phase(event, step.phase);
            const auto start = day + minute * 60;
            const auto finish = day + std::min(1440, minute + step.minutes) * 60;
            if (!result.empty() && previous_clipped && result.back().ends_at == start
                && result.back().phase == &phase) {
                result.back().ends_at = finish;
            } else {
                result.push_back({&event, &phase, start, finish});
            }
            previous_clipped = minute + step.minutes > 1440;
            minute += step.minutes;
        };
        for (const auto& step : *partial) {
            if (minute >= 1440) break;
            append(step);
        }
        if (minute < 1440 && event.pattern.empty()) throw std::invalid_argument("Incomplete event day");
        while (minute < 1440) {
            for (const auto& step : event.pattern) {
                if (minute >= 1440) break;
                append(step);
            }
        }
    }
    result.erase(std::remove_if(result.begin(), result.end(), [&](const auto& window) {
        return window.ends_at <= begin || window.starts_at >= end;
    }), result.end());
    return result;
}

std::string EventTimeText(std::int64_t time, std::int64_t now, Language language) {
    const auto difference = (BeijingDayStart(time) - BeijingDayStart(now)) / kDaySeconds;
    std::string date;
    if (difference >= -1 && difference <= 1) {
        date = DayLabel(time, now, language);
    } else {
        const auto adjusted = static_cast<std::time_t>(time + kBeijingOffset);
        std::tm calendar{};
#ifdef _WIN32
        const bool valid = gmtime_s(&calendar, &adjusted) == 0;
#else
        const bool valid = gmtime_r(&adjusted, &calendar) != nullptr;
#endif
        char buffer[32]{};
        if (valid) std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &calendar);
        date = buffer;
    }
    return date + " " + ClockText(time);
}

std::string EventCopyText(const EventWindow& window, std::int64_t now, Language language) {
    std::string text = EventName(*window.event, language) + u8" · " + PhaseName(*window.phase, language)
        + u8" · " + EventTimeText(window.starts_at, now, language)
        + "-" + EventTimeText(window.ends_at, now, language) + " (UTC+8)";
    if (!window.phase->waypoints.empty()) text += " " + window.phase->waypoints;
    if (window.event->approximate) text += language == Language::CN ? u8" · 推测时段，不保证刷新" : " | Approximate, not guaranteed";
    return text;
}
} // namespace world_timer
