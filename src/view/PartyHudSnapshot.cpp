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
    for (int i = 0; i < party.getMemberCount(); ++i) {
        const auto m = party.getMember(i);
        if (!m) continue;
        slots.push_back(PartyHudSlot{
            static_cast<std::size_t>(i + 1),
            m->getName(),
            m->getClass(),
            m->getClassSlug(),
            m->getSpecialization(),
            m->getRow(),
            m->getLevel(),
            m->getHp(),
            m->getMaxHp(),
            m->getSpellSlots(),
            m->getMaxSpellSlots(),
            m->getAc(),
            m->isDead(),
            m->isGuarding(),
            m->getPoisonTurns(),
            m->getParalysisTurns(),
            m->getBurnTurns(),
            m->getBleedTurns(),
            m->getFrostTurns(),
            m->getBlessTurns(),
            m->getWardTurns(),
            m->getTauntTurns(),
            m->getTalentPoints()
        });
    }
    return PartyHudSnapshot(std::move(slots));
}

} // namespace crawl
