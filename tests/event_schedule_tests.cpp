#include "event_schedule.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace world_timer;
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
const EventRow& Row(const std::string& id) {
    const auto row = std::find_if(Events().begin(), Events().end(), [&](const auto& value) { return value.id == id; });
    if (row == Events().end()) throw std::runtime_error("Missing row: " + id);
    return *row;
}
int main() {
    try {
        Check(Events().size() == 42, "Source snapshot has 42 non-seasonal rows");
        const auto& groups = EventGroups();
        const std::size_t expected_counts[] = {42, 5, 3, 2, 5, 7, 5, 9, 3, 5, 11};
        Check(groups.size() == 11, "All eleven source shortcuts are available");
        for (std::size_t i = 0; i < groups.size(); ++i) {
            const auto& group = groups[i];
            Check(!group.name.empty() && !group.name_en.empty(), "Shortcut has Chinese and English labels");
            const auto count = std::count_if(Events().begin(), Events().end(),
                [&](const auto& row) { return EventInGroup(row, group); });
            Check(static_cast<std::size_t>(count) == expected_counts[i], "Shortcut membership matches source page");
            for (const auto& id : group.row_ids) Row(id);
        }
        for (const auto& row : Events()) {
            Check(std::any_of(groups.begin() + 1, groups.end(),
                [&](const auto& group) { return EventInGroup(row, group); }), "Every event has a shortcut");
        }
        Check(EventInGroup(Row("la"), groups[5]) && EventInGroup(Row("la"), groups[9])
              && EventInGroup(Row("la"), groups[10]), "Ley-line anomaly appears in Tyria, Special and Meta train");
        Check(EventInGroup(Row("LakeDoric"), groups[6]) && EventInGroup(Row("ThunderheadPeaks"), groups[7]),
              "Living World maps follow the source expansion shortcuts");
        const auto at_start = EventWindows(Row("cankc"), 0, 1);
        Check(at_start.size() == 1 && at_start[0].phase->active && at_start[0].phase->id == 1,
              "Kaineng begins at UTC 00:00 / Beijing 08:00");
        Check(ClockText(at_start[0].starts_at) == "08:00", "UTC source converts to Beijing exactly once");
        const auto at_end = EventWindows(Row("cankc"), 1800, 1801);
        Check(at_end.size() == 1 && !at_end[0].phase->active, "Phase intervals are half-open at exact end");
        const auto night = EventWindows(Row("daynight"), -60, 60);
        Check(night.size() == 1 && night[0].phase->id == 3 && night[0].starts_at == -900
              && night[0].ends_at == 1500, "Night phase retains its start across UTC reset");
        constexpr std::int64_t anchor = 1525910400;
        Check(EventWindows(Row("Zeghai"), anchor + 900, anchor + 901)[0].phase->id == 2,
              "Vendor rotation matches the source anchor");
        Check(!EventWindows(Row("Zeghai"), anchor + 2400, anchor + 2401)[0].phase->active,
              "Vendor departs after 25 minutes");
        Check(EventWindows(Row("Zeghai"), anchor + kDaySeconds + 900, anchor + kDaySeconds + 901)[0].phase->id == 11,
              "Vendor rotation advances across daily reset");
        const auto copy = EventCopyText(at_start[0], 0, Language::CN);
        Check(copy.find(u8"新凯宁城") != std::string::npos && copy.find("[&BBkNAAA=]") != std::string::npos,
              "Chinese event copy includes localized map and waypoint");
        const auto english = EventCopyText(at_start[0], 0, Language::EN);
        Check(english.find("New Kaineng City") != std::string::npos && english.find("Kaineng Blackout") != std::string::npos,
              "English event copy switches map and phase");
        Check(EventCopyText(EventWindows(Row("BloodstoneCrazed"),0,1)[0],0,Language::EN).find("not guaranteed") != std::string::npos,
              "Sampled bloodstone rotation is explicitly approximate");
        Check(EventTimeText(3 * kDaySeconds, 0, Language::EN) == "1970-01-04 08:00",
              "Distant dates are explicit rather than mislabeled tomorrow");
        // Exercise all seven weekday schedules, Beijing midnight and 08:00 reset,
        // with a complete 24-hour viewport on each boundary.
        constexpr std::int64_t monday = 1795392000; // 2026-11-23 00:00 UTC
        for (int day=0; day<7; ++day) for (const auto offset : {-1, 0, 1, 57599, 57600, 86399}) {
            const auto begin = monday + day * kDaySeconds + offset;
            for (const auto& row : Events()) {
                const auto windows = EventWindows(row, begin, begin + kDaySeconds);
                Check(!windows.empty() && windows.front().starts_at <= begin
                      && windows.back().ends_at >= begin + kDaySeconds, "Every row covers the entire viewport");
                for (std::size_t i=0; i<windows.size(); ++i) {
                    const auto& window=windows[i];
                    Check(window.ends_at > window.starts_at && window.phase && window.event == &row, "Valid phase interval");
                    if (i) Check(windows[i-1].ends_at == window.starts_at, "No overlap or gaps at reset");
                    Check(!PhaseName(*window.phase,Language::EN).empty(), "Every phase has an English label");
                }
            }
        }
        const auto monday_phase = EventWindows(Row("JoKoAttack"), monday + 1800, monday + 1801)[0].phase->id;
        const auto tuesday_phase = EventWindows(Row("JoKoAttack"), monday + kDaySeconds + 1800,
                                                monday + kDaySeconds + 1801)[0].phase->id;
        Check(monday_phase != tuesday_phase, "Joko weekday rotation advances without a page refresh");
        std::cout << "PASS: 42 rows, UTC+8, phase boundaries, seven weekdays, vendor rotation and localized copy\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return EXIT_FAILURE; }
}
