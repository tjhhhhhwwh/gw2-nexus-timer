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
        Check(PreviousOccurrence(boss, midnight).starts_at == midnight, "Previous includes exact start");
        Check(PreviousOccurrence(boss, midnight - 1).starts_at == midnight - 60, "Previous across midnight");
        Check(DayLabel(midnight - 1, midnight) == u8"昨天", "Previous-day copy label");
        Check(DayLabel(midnight - 1, midnight, Language::EN) == "Yesterday", "English previous-day label");
        Boss evening{"evening", u8"晚间首领", u8"地图", "[&BH4BAAA=]", "", {1080}, "Evening", "Map", ""};
        const std::vector<Boss> evening_bosses{evening};
        const auto just_started = CardOccurrences(evening_bosses, midnight + 18 * 3600);
        Check(just_started[0].recent, "Card retains the occurrence at its exact start");
        Check(just_started[0].next.starts_at == midnight + kDaySeconds + 18 * 3600,
              "Card next points to following start at the exact boundary");
        const auto within_window = CardOccurrences(evening_bosses, midnight + 18 * 3600 + 899);
        Check(within_window[0].recent, "Recent card visible through 14:59");
        const auto expired = CardOccurrences(evening_bosses, midnight + 18 * 3600 + 900);
        Check(!expired[0].recent, "Recent card expires at 15:00");
        Boss late_boss{"late", "Late", "Map", "[&BH4BAAA=]", "", {1439}, "Late", "Map", ""};
        const std::vector<Boss> late_bosses{late_boss};
        const auto rollover = CardOccurrences(late_bosses, midnight + kDaySeconds + 60);
        Check(rollover[0].recent && rollover[0].previous.starts_at == midnight + 1439 * 60,
              "Recent display survives Beijing midnight");
        Check(CopyText(rollover[0].previous, midnight + kDaySeconds + 60) ==
              u8"Late · 昨天 23:59 · Map [&BH4BAAA=]", "Recent occurrence copies the correct previous day");
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
        // The supplied reference at 18:13:48 keeps the 18:00 / 18:10 starts
        // visible before the 18:15 and 18:25 upcoming bosses.
        const auto reference = CardOccurrences(Bosses(), midnight + 18 * 3600 + 13 * 60 + 48);
        Check(reference[0].next.boss->id == "b08" && reference[0].recent, "18:00 Shatterer still displayed");
        Check(reference[1].next.boss->id == "b22" && reference[1].recent, "18:00 Kaineng still displayed");
        Check(reference[2].next.boss->id == "b14" && reference[2].recent, "18:10 night event still displayed");
        Check(reference[3].next.boss->id == "b01" && !reference[3].recent, "18:15 Shaman follows recent starts");
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
            const auto cards = CardOccurrences(Bosses(), now);
            Check(cards.size() == Bosses().size(), "Every boss has one card");
            bool seen_future = false;
            std::int64_t last_recent = now - kDaySeconds;
            std::int64_t last_next = now;
            for (const auto& card : cards) {
                Check(card.previous.starts_at <= now && card.next.starts_at > now, "Card brackets current time");
                if (card.recent) {
                    Check(!seen_future && card.previous.starts_at >= last_recent, "Recent cards first in time order");
                    last_recent = card.previous.starts_at;
                } else {
                    seen_future = true;
                    Check(card.next.starts_at >= last_next, "Future cards in time order");
                    last_next = card.next.starts_at;
                }
            }
        }
        std::cout << "PASS: midnight rollover, UTC+8, exact starts, ordering, copy text, 24 bosses / 246 times\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
