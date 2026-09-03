#include "view/PartyHudSnapshot.hpp"

#include "model/Party.hpp"

namespace crawl {

PartyHudSnapshot::PartyHudSnapshot(std::vector<PartyHudSlot> slots)
    : m_slots(std::move(slots)) {}

const std::vector<PartyHudSlot>& PartyHudSnapshot::slots() const noexcept {
    return m_slots;
}

PartyHudSnapshot buildPartyHudSnapshot(const Party& party) {
    std::vector<PartyHudSlot> slots;
    slots.reserve(static_cast<std::size_t>(party.getMemberCount()));
    for (int index = 0; index < party.getMemberCount(); ++index) {
        const auto member = party.getMember(index);
        if (!member) continue;
        slots.push_back({
            static_cast<std::size_t>(index + 1),
            member->getName(),
            member->getClass(),
            member->getHp(),
            member->getMaxHp(),
            member->isDead(),
            member->getPoisonTurns(),
            member->getParalysisTurns(),
            member->getBlessTurns()
        });
    }
    return PartyHudSnapshot(std::move(slots));
}

} // namespace crawl
