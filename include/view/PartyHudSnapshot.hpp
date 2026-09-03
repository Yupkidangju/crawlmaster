#ifndef PARTY_HUD_SNAPSHOT_HPP
#define PARTY_HUD_SNAPSHOT_HPP

#include "model/Character.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace crawl {

class Party;

struct PartyHudSlot {
    std::size_t slotNumber;
    std::string name;
    CharacterClass characterClass;
    int hp;
    int maxHp;
    bool isDead;
    int poisonTurns;
    int paralysisTurns;
    int blessTurns;
};

class PartyHudSnapshot {
public:
    explicit PartyHudSnapshot(std::vector<PartyHudSlot> slots);
    const std::vector<PartyHudSlot>& slots() const noexcept;

private:
    std::vector<PartyHudSlot> m_slots;
};

PartyHudSnapshot buildPartyHudSnapshot(const Party& party);

} // namespace crawl

#endif // PARTY_HUD_SNAPSHOT_HPP
