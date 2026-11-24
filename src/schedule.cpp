#include "schedule.h"
#include "boss_data.generated.h"

#include <algorithm>
#include <cstdio>
#include <stdexcept>

namespace world_timer {
const std::vector<Boss>& Bosses() { return generated::kBosses; }
const char* SnapshotDate() { return generated::kSnapshotDate; }

std::int64_t BeijingDayStart(std::int64_t now) {
    const auto shifted = now + kBeijingOffset;
    const auto days = shifted / kDaySeconds - (shifted % kDaySeconds < 0 ? 1 : 0);
    return days * kDaySeconds - kBeijingOffset;
}

Occurrence NextOccurrence(const Boss& boss, std::int64_t now) {
    if (boss.minutes.empty()) throw std::invalid_argument("Boss has no scheduled times");
    const auto day = BeijingDayStart(now);
    std::int64_t next = day + kDaySeconds * 2;
    for (const int minute : boss.minutes) {
        if (minute < 0 || minute >= 1440) throw std::invalid_argument("Invalid schedule minute");
        auto start = day + minute * 60;
        if (start < now) start += kDaySeconds;
        next = std::min(next, start);
    }
    return {&boss, next};
}

std::vector<Occurrence> Upcoming(const std::vector<Boss>& bosses, std::int64_t now) {
    std::vector<Occurrence> result;
    result.reserve(bosses.size());
    for (const auto& boss : bosses) result.push_back(NextOccurrence(boss, now));
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        if (a.starts_at != b.starts_at) return a.starts_at < b.starts_at;
        return a.boss->id < b.boss->id;
    });
    return result;
}

Occurrence PreviousOccurrence(const Boss& boss, std::int64_t now) {
    if (boss.minutes.empty()) throw std::invalid_argument("Boss has no scheduled times");
    const auto day = BeijingDayStart(now);
    std::int64_t previous = day - kDaySeconds * 2;
    for (const int minute : boss.minutes) {
        if (minute < 0 || minute >= 1440) throw std::invalid_argument("Invalid schedule minute");
        auto start = day + minute * 60;
        if (start > now) start -= kDaySeconds;
        previous = std::max(previous, start);
    }
    return {&boss, previous};
}

std::vector<CardOccurrence> CardOccurrences(const std::vector<Boss>& bosses, std::int64_t now) {
    std::vector<CardOccurrence> result;
    result.reserve(bosses.size());
    for (const auto& boss : bosses) {
        const auto previous = PreviousOccurrence(boss, now);
        // At an exact start, "previous" is the start just reached, and "next"
        // must point to the following start rather than the same occurrence.
        result.push_back({previous, NextOccurrence(boss, now + 1),
                          now - previous.starts_at < kRecentDisplaySeconds});
    }
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
        if (a.recent != b.recent) return a.recent;
        const auto a_time = a.recent ? a.previous.starts_at : a.next.starts_at;
        const auto b_time = b.recent ? b.previous.starts_at : b.next.starts_at;
        if (a_time != b_time) return a_time < b_time;
        return a.next.boss->id < b.next.boss->id;
    });
    return result;
}

std::string ClockText(std::int64_t now) {
    const auto seconds = now - BeijingDayStart(now);
    char text[16];
    std::snprintf(text, sizeof(text), "%02d:%02d", static_cast<int>(seconds / 3600),
                  static_cast<int>((seconds % 3600) / 60));
    return text;
}

const std::string& BossName(const Boss& boss, Language language) {
    return language == Language::EN && !boss.name_en.empty() ? boss.name_en : boss.name;
}

const std::string& MapName(const Boss& boss, Language language) {
    return language == Language::EN && !boss.map_en.empty() ? boss.map_en : boss.map;
}

std::string DayLabel(std::int64_t starts_at, std::int64_t now, Language language) {
    const auto day = BeijingDayStart(starts_at);
    const auto today = BeijingDayStart(now);
    if (language == Language::EN) return day < today ? "Yesterday" : day == today ? "Today" : "Tomorrow";
    return day < today ? u8"昨天" : day == today ? u8"今天" : u8"明天";
}

std::string Countdown(std::int64_t seconds) {
    seconds = std::max<std::int64_t>(0, seconds);
    char text[32];
    std::snprintf(text, sizeof(text), "%02lld:%02lld:%02lld",
                  static_cast<long long>(seconds / 3600),
                  static_cast<long long>((seconds % 3600) / 60),
                  static_cast<long long>(seconds % 60));
    return text;
}

std::string CopyText(const Occurrence& occurrence, std::int64_t now, Language language) {
    const auto& boss = *occurrence.boss;
    return BossName(boss, language) + u8" · " + DayLabel(occurrence.starts_at, now, language) + " "
        + ClockText(occurrence.starts_at) + u8" · " + MapName(boss, language) + " " + boss.waypoint;
}
} // namespace world_timer
