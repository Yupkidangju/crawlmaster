#ifndef BESTIARY_HPP
#define BESTIARY_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace crawl {

struct BestiaryEntry {
    int encountered = 0;
    int defeated = 0;
    bool weaknessRevealed = false;
};

class Bestiary {
public:
    void recordEncounter(const std::string& monsterId) {
        auto& e = m_entries[monsterId];
        e.encountered += 1;
    }

    void recordDefeat(const std::string& monsterId) {
        auto& e = m_entries[monsterId];
        e.encountered = std::max(e.encountered, 1);
        e.defeated += 1;
        e.weaknessRevealed = true;
    }

    void revealWeakness(const std::string& monsterId) {
        auto& e = m_entries[monsterId];
        e.encountered = std::max(e.encountered, 1);
        e.weaknessRevealed = true;
    }

    [[nodiscard]] BestiaryEntry getEntry(const std::string& monsterId) const {
        const auto it = m_entries.find(monsterId);
        return it != m_entries.end() ? it->second : BestiaryEntry{};
    }

    [[nodiscard]] bool isDiscovered(const std::string& monsterId) const {
        const auto it = m_entries.find(monsterId);
        return it != m_entries.end() && it->second.encountered > 0;
    }

    [[nodiscard]] int discoveredCount() const {
        int count = 0;
        for (const auto& [id, e] : m_entries) {
            if (e.encountered > 0) ++count;
        }
        return count;
    }

    [[nodiscard]] int totalDefeated() const {
        int sum = 0;
        for (const auto& [id, e] : m_entries) sum += e.defeated;
        return sum;
    }

    void clear() { m_entries.clear(); }

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json j = nlohmann::json::object();
        for (const auto& [id, entry] : m_entries) {
            j[id] = {
                {"encountered", entry.encountered},
                {"defeated", entry.defeated},
                {"weaknessRevealed", entry.weaknessRevealed}
            };
        }
        return j;
    }

    static Bestiary fromJson(const nlohmann::json& j) {
        Bestiary b;
        if (!j.is_object()) return b;
        for (const auto& [id, val] : j.items()) {
            if (!val.is_object()) continue;
            BestiaryEntry e;
            e.encountered = std::max(0, val.value("encountered", 0));
            e.defeated = std::max(0, val.value("defeated", 0));
            e.weaknessRevealed = val.value("weaknessRevealed", e.defeated > 0);
            b.m_entries[id] = e;
        }
        return b;
    }

private:
    std::unordered_map<std::string, BestiaryEntry> m_entries;
};

} // namespace crawl

#endif // BESTIARY_HPP
