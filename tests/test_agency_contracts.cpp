#include "core/SessionRng.hpp"
#include "model/Character.hpp"
#include "model/CombatActionRules.hpp"
#include "model/ConcreteItems.hpp"
#include "model/DungeonMap.hpp"
#include "model/ItemFactory.hpp"
#include "model/Party.hpp"
#include "model/RecruitmentDraft.hpp"

#include <iostream>

#define CHECK(expr)                                                                  \
    do {                                                                             \
        if (!(expr)) {                                                               \
            std::cerr << "[FAIL] AgencyContract: " #expr " (" << __LINE__ << ")\n"; \
            return 1;                                                                \
        }                                                                            \
    } while (false)

int main() {
    using namespace crawl;
    SessionRng rng(77777U, 0);

    // 1. 무효 소모품 비소비 규칙 검증
    Character hero("Aldric", CharacterClass::WARRIOR);
    auto healPot = std::dynamic_pointer_cast<ConsumableItem>(ItemFactory::createItem("pot_heal"));
    auto cureScr = std::dynamic_pointer_cast<ConsumableItem>(ItemFactory::createItem("scr_cure"));
    auto elixir = std::dynamic_pointer_cast<ConsumableItem>(ItemFactory::createItem("pot_elixir"));
    auto fireBomb = std::dynamic_pointer_cast<ConsumableItem>(ItemFactory::createItem("bomb_fire"));
    CHECK(!CombatActionRules::canUseConsumable(*healPot, hero));
    CHECK(!CombatActionRules::canUseConsumable(*cureScr, hero));
    CHECK(CombatActionRules::canUseConsumable(*fireBomb, hero));

    hero.takeDamage(hero.getMaxHp());
    CHECK(hero.isDead());
    CHECK(!CombatActionRules::canUseConsumable(*healPot, hero));
    CHECK(CombatActionRules::canUseConsumable(*elixir, hero));

    // 2. RecruitmentDraft 확정형 생성 검증
    Party party;
    party.removeMember(3);
    RecruitmentDraft draft(rng);
    CHECK(draft.setName("Kaelen"));
    CHECK(draft.setAge(29));
    CHECK(draft.setClass(CharacterClass::ROGUE));
    CHECK(!draft.isReady()); // 성별 미선택 + 잔여 포인트 미배분 상태에서는 확정 불가
    CHECK(draft.setGender(Gender::FEMALE));
    const Ability order[] = {
        Ability::DEXTERITY, Ability::STRENGTH, Ability::CONSTITUTION,
        Ability::INTELLIGENCE, Ability::WISDOM, Ability::CHARISMA
    };
    while (draft.remainingPoints() > 0) {
        bool progressed = false;
        for (Ability ab : order) {
            if (draft.increase(ab)) { progressed = true; break; }
        }
        CHECK(progressed);
    }
    CHECK(draft.isReady());
    CHECK(draft.confirm(party));
    CHECK(party.getMemberCount() == 4);
    CHECK(!draft.confirm(party)); // 일회성 확정

    // 3. 잠긴 문 해제 및 비밀 벽 개방 검증
    DungeonMap map;
    map.generate(12345U, 1);
    map.setTile(3, 3, TileType::LOCKED_GATE);
    CHECK(!map.isWalkable(3, 3));
    CHECK(map.unlockAllGates());
    CHECK(map.isWalkable(3, 3));

    map.setTile(4, 4, TileType::SECRET_WALL);
    CHECK(!map.isWalkable(4, 4));
    CHECK(map.revealSecretWall(4, 4));
    CHECK(map.isWalkable(4, 4));

    std::cout << "[PASS] AgencyContractTests passed.\n";
    return 0;
}
