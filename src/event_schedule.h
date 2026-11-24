#pragma once
#include "schedule.h"
#include <array>

namespace world_timer {
struct EventPhase {
    int id;
    std::string name;
    std::string name_en;
    std::string waypoints;
    std::array<int, 3> color;
    bool active;
};
struct PhaseStep { int phase; int minutes; };
struct EventRow {
    std::string id;
    std::string name;
    std::string name_en;
    std::string category;
    std::string category_en;
    std::vector<EventPhase> phases;
    std::vector<PhaseStep> partial;
    std::vector<PhaseStep> pattern;
    // Monday first. The source's rotating schedules reset at 00:00 UTC / 08:00 Beijing.
    std::vector<std::vector<PhaseStep>> weekly_partials;
    bool zeghai_rotation;
    bool approximate;
};
struct EventWindow {
    const EventRow* event;
    const EventPhase* phase;
    std::int64_t starts_at;
    std::int64_t ends_at;
};
struct EventGroup {
    std::string id;
    std::string name;
    std::string name_en;
    std::vector<std::string> row_ids;
};
const std::vector<EventRow>& Events();
const std::vector<EventGroup>& EventGroups();
bool EventInGroup(const EventRow& event, const EventGroup& group);
const char* EventSnapshotDate();
std::vector<EventWindow> EventWindows(const EventRow& event, std::int64_t begin, std::int64_t end);
const std::string& EventName(const EventRow& event, Language language);
const std::string& EventCategory(const EventRow& event, Language language);
const std::string& PhaseName(const EventPhase& phase, Language language);
std::string EventTimeText(std::int64_t time, std::int64_t now, Language language);
std::string EventCopyText(const EventWindow& window, std::int64_t now, Language language);
} // namespace world_timer
