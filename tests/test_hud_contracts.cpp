#include "model/Character.hpp"
#include "model/Party.hpp"
#include "view/PartyHudSnapshot.hpp"

#include <iostream>

#define CHECK(expr)                                                               \
    do {                                                                          \
        if (!(expr)) {                                                            \
            std::cerr << "[FAIL] HudContract: " #expr " (" << __LINE__ << ")\n"; \
            return 1;                                                             \
        }                                                                         \
    } while (false)

int main() {
    using namespace crawl;
    Party party;
    CHECK(party.getMemberCount() == 4);
    party.getMember(0)->takeDamage(5);
    party.getMember(0)->setGuarding(true);
    party.getMember(1)->setPoison(3);
    party.getMember(2)->setRow(CombatRow::BACK);
    party.getMember(3)->applyBless(4);

    const auto snap = buildPartyHudSnapshot(party);
    CHECK(snap.slots().size() == 4);
    CHECK(snap.slots()[0].isGuarding);
    CHECK(snap.slots()[0].hp == party.getMember(0)->getHp());
    CHECK(snap.slots()[1].poisonTurns == 3);
    CHECK(snap.slots()[2].row == CombatRow::BACK);
    CHECK(snap.slots()[3].blessTurns == 4);

    // 원본 파티 변경 후에도 불변 스냅샷 유지 검증
    const int oldHp = snap.slots()[0].hp;
    party.getMember(0)->takeDamage(4);
    CHECK(snap.slots()[0].hp == oldHp);

    std::cout << "[PASS] HudContractTests passed.\n";
    return 0;
}
