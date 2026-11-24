#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace world_timer {
constexpr std::int64_t kDaySeconds = 86400;
constexpr std::int64_t kBeijingOffset = 8 * 3600;
enum class Language { CN, EN };

struct Boss {
    std::string id;
    std::string name;
    std::string map;
    std::string waypoint;
    std::string zone;
    std::vector<int> minutes;
    std::string name_en;
    std::string map_en;
    std::string zone_en;
};

struct Occurrence {
    const Boss* boss;
    std::int64_t starts_at;
};

// A display window after a scheduled start, never live map-instance status.
constexpr std::int64_t kRecentDisplaySeconds = 15 * 60;
struct CardOccurrence {
    Occurrence previous;
    Occurrence next;
    bool recent;
};

const std::vector<Boss>& Bosses();
const char* SnapshotDate();
std::int64_t BeijingDayStart(std::int64_t unix_seconds);
Occurrence NextOccurrence(const Boss& boss, std::int64_t now);
Occurrence PreviousOccurrence(const Boss& boss, std::int64_t now);
std::vector<CardOccurrence> CardOccurrences(const std::vector<Boss>& bosses, std::int64_t now);
std::vector<Occurrence> Upcoming(const std::vector<Boss>& bosses, std::int64_t now);
std::string ClockText(std::int64_t unix_seconds);
const std::string& BossName(const Boss& boss, Language language);
const std::string& MapName(const Boss& boss, Language language);
std::string DayLabel(std::int64_t starts_at, std::int64_t now, Language language = Language::CN);
std::string Countdown(std::int64_t seconds);
std::string CopyText(const Occurrence& occurrence, std::int64_t now, Language language = Language::CN);
} // namespace world_timer
