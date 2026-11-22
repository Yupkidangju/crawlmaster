#ifndef PARTY_HUD_SNAPSHOT_HPP
#define PARTY_HUD_SNAPSHOT_HPP

#include "model/Character.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace crawl {

class Party;

struct PartyHudSlot {
    std::size_t slotNumber = 1;
    std::string name;
    CharacterClass characterClass = CharacterClass::WARRIOR;
    std::string classSlug = "warrior";
    Specialization specialization = Specialization::NONE;
    CombatRow row = CombatRow::FRONT;
    int level = 1;
    int hp = 0;
    int maxHp = 1;
    int spellSlots = 0;
    int maxSpellSlots = 0;
    int ac = 10;
    bool isDead = false;
    bool isGuarding = false;
    int poisonTurns = 0;
    int paralysisTurns = 0;
    int burnTurns = 0;
    int bleedTurns = 0;
    int frostTurns = 0;
    int blessTurns = 0;
    int wardTurns = 0;
    int tauntTurns = 0;
    int talentPoints = 0;
};

class PartyHudSnapshot {
public:
    explicit PartyHudSnapshot(std::vector<PartyHudSlot> slots);
    [[nodiscard]] const std::vector<PartyHudSlot>& slots() const noexcept;

private:
    std::vector<PartyHudSlot> m_slots;
};

PartyHudSnapshot buildPartyHudSnapshot(const Party& party);

} // namespace crawl

#endif // PARTY_HUD_SNAPSHOT_HPP
