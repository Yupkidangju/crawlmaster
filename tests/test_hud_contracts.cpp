#include "model/Character.hpp"
#include "model/Party.hpp"
#include "view/PartyHudSnapshot.hpp"

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

int g_failureCount = 0;

void check(bool condition, const char* expression, const char* file, int line) {
    if (condition) {
        return;
    }

    ++g_failureCount;
    std::cerr << "[Failure] " << file << ':' << line
              << ": CHECK(" << expression << ")\n";
}

#define CHECK(condition) check(static_cast<bool>(condition), #condition, __FILE__, __LINE__)

void checkSlotMatchesCharacter(const crawl::PartyHudSlot& slot,
                               std::size_t expectedSlotNumber,
                               const crawl::Character& character) {
    CHECK(slot.slotNumber == expectedSlotNumber);
    CHECK(slot.name == character.getName());
    CHECK(slot.characterClass == character.getClass());
    CHECK(slot.hp == character.getHp());
    CHECK(slot.maxHp == character.getMaxHp());
    CHECK(slot.isDead == character.isDead());
    CHECK(slot.poisonTurns == character.getPoisonTurns());
    CHECK(slot.paralysisTurns == character.getParalysisTurns());
    CHECK(slot.blessTurns == character.getBlessTurns());
}

void testEmptyPartyProducesNoHudSlots() {
    crawl::Party party;
    const crawl::Party& domainParty = party;

    const crawl::PartyHudSnapshot snapshot = crawl::buildPartyHudSnapshot(domainParty);

    CHECK(domainParty.getMemberCount() == 0);
    CHECK(snapshot.slots().empty());
}

void testSingleMemberSlotMirrorsDomainState() {
    crawl::Party party;
    auto member = std::make_shared<crawl::Character>("Ariadne", crawl::CharacterClass::MAGE);
    member->takeDamage(1);
    member->setPoison(3);
    member->setParalysis(2);
    member->applyBless(4);
    CHECK(party.addMember(member));

    const crawl::PartyHudSnapshot snapshot = crawl::buildPartyHudSnapshot(party);

    CHECK(snapshot.slots().size() == 1);
    if (snapshot.slots().size() == 1) {
        checkSlotMatchesCharacter(snapshot.slots()[0], 1, *member);
    }

    const int capturedHp = member->getHp();
    const int capturedPoisonTurns = member->getPoisonTurns();
    const int capturedParalysisTurns = member->getParalysisTurns();
    const int capturedBlessTurns = member->getBlessTurns();
    member->takeDamage(1);
    member->setPoison(9);
    member->setParalysis(8);
    member->applyBless(7);
    party.removeMember(0);

    CHECK(snapshot.slots().size() == 1);
    if (snapshot.slots().size() == 1) {
        CHECK(snapshot.slots()[0].hp == capturedHp);
        CHECK(snapshot.slots()[0].poisonTurns == capturedPoisonTurns);
        CHECK(snapshot.slots()[0].paralysisTurns == capturedParalysisTurns);
        CHECK(snapshot.slots()[0].blessTurns == capturedBlessTurns);
    }
}

void testFourMemberSlotsPreserveOrderAndAllStates() {
    crawl::Party party;
    const std::vector<std::shared_ptr<crawl::Character>> members = {
        std::make_shared<crawl::Character>("Borin", crawl::CharacterClass::WARRIOR),
        std::make_shared<crawl::Character>("Cyra", crawl::CharacterClass::MAGE),
        std::make_shared<crawl::Character>("Dain", crawl::CharacterClass::ROGUE),
        std::make_shared<crawl::Character>("Edda", crawl::CharacterClass::CLERIC),
    };

    members[0]->takeDamage(2);
    members[0]->setPoison(5);
    members[1]->setParalysis(3);
    members[2]->applyBless(6);
    members[3]->setPoison(1);
    members[3]->setParalysis(2);
    members[3]->applyBless(3);
    members[3]->takeDamage(members[3]->getMaxHp());

    for (const auto& member : members) {
        CHECK(party.addMember(member));
    }

    const crawl::PartyHudSnapshot snapshot = crawl::buildPartyHudSnapshot(party);

    CHECK(snapshot.slots().size() == 4);
    if (snapshot.slots().size() == members.size()) {
        for (std::size_t index = 0; index < members.size(); ++index) {
            checkSlotMatchesCharacter(snapshot.slots()[index], index + 1, *members[index]);
        }
    }
}

} // namespace

int main() {
    testEmptyPartyProducesNoHudSlots();
    testSingleMemberSlotMirrorsDomainState();
    testFourMemberSlotsPreserveOrderAndAllStates();

    if (g_failureCount != 0) {
        std::cerr << g_failureCount << " HUD contract check(s) failed.\n";
        return 1;
    }

    std::cout << "All HUD snapshot contract checks passed.\n";
    return 0;
}
