#include "schedule.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

using namespace world_timer;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    try {
        // 1970-01-01 00:00 Beijing. These tests never consult the host timezone.
        constexpr std::int64_t midnight = -kBeijingOffset;
        Boss boss{"test", u8"测试首领", u8"测试地图", "[&BH4BAAA=]", u8"泰瑞亚", {0, 15, 1439},
                  "Test Boss", "Test Map", "Tyria"};
        Check(BeijingDayStart(midnight) == midnight, "Beijing midnight boundary");
        Check(BeijingDayStart(midnight - 1) == midnight - kDaySeconds, "Previous day boundary");
        Check(ClockText(0) == "08:00", "Timezone must be UTC+8");
        Check(NextOccurrence(boss, midnight).starts_at == midnight, "Exact start is included");
        Check(NextOccurrence(boss, midnight + 1).starts_at == midnight + 900, "Past event advances");
        const auto late = NextOccurrence(boss, midnight + kDaySeconds - 1);
        Check(late.starts_at == midnight + kDaySeconds, "Last second rolls to tomorrow");
        Check(DayLabel(late.starts_at, midnight + kDaySeconds - 1) == u8"明天", "Tomorrow label");
        Check(CopyText(late, midnight + kDaySeconds - 1) ==
              u8"测试首领 · 明天 00:00 · 测试地图 [&BH4BAAA=]", "Unicode copy text and waypoint");
        Check(CopyText(late, midnight + kDaySeconds - 1, Language::EN) ==
              u8"Test Boss · Tomorrow 00:00 · Test Map [&BH4BAAA=]", "English copy text and waypoint");
        Check(DayLabel(midnight, midnight, Language::EN) == "Today", "English today label");
        Check(BossName(boss, Language::CN) == u8"测试首领", "CN mode uses Chinese name");
        Check(Countdown(3661) == "01:01:01", "Countdown formatting");
        Check(Countdown(-1) == "00:00:00", "No negative countdown");
        bool rejected = false;
        try { Boss empty; NextOccurrence(empty, 0); } catch (const std::invalid_argument&) { rejected = true; }
        Check(rejected, "Empty schedules must be rejected");
        Check(Bosses().size() == 24, "Imported boss count");
        std::size_t total = 0;
        for (const auto& b : Bosses()) {
            total += b.minutes.size();
            Check(!b.name_en.empty() && !b.map_en.empty(), "Every boss has English labels");
        }
        Check(total == 246, "Imported time count");
        for (const auto now : {midnight - 1, midnight, midnight + 1,
                               midnight + 86399, std::int64_t{1795449600}}) {
            auto rows = Upcoming(Bosses(), now);
            Check(rows.size() == Bosses().size(), "Every boss has a next occurrence");
            Check(std::is_sorted(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
                return a.starts_at < b.starts_at;
            }), "Occurrences sorted by time");
            for (const auto& row : rows) {
                Check(row.starts_at >= now && row.starts_at < now + kDaySeconds, "Next occurrence within 24h");
                Check(CopyText(row, now).find(row.boss->waypoint) != std::string::npos, "Every copy includes waypoint");
            }
        }
        std::cout << "PASS: midnight rollover, UTC+8, exact starts, ordering, copy text, 24 bosses / 246 times\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
