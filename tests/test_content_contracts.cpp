#include "core/SessionRng.hpp"
#include "model/Bestiary.hpp"
#include "model/DungeonWorld.hpp"
#include "model/ItemFactory.hpp"
#include "model/MonsterFactory.hpp"
#include "model/Quest.hpp"
#include "model/SkillFactory.hpp"

#include <iostream>

#define CHECK(expr)                                                                   \
    do {                                                                              \
        if (!(expr)) {                                                                \
            std::cerr << "[FAIL] ContentContract: " #expr " (" << __LINE__ << ")\n"; \
            return 1;                                                                 \
        }                                                                             \
    } while (false)

int main() {
    using namespace crawl;

    // 1. 43종 아이템 레지스트리 검증
    const auto itemIds = ItemFactory::getRegisteredIds();
    CHECK(itemIds.size() >= 40);
    for (const auto& id : itemIds) {
        auto item = ItemFactory::createItem(id);
        CHECK(item != nullptr);
        CHECK(item->getId() == id);
        auto roundTrip = ItemFactory::createItemFromJson(item->toJson());
        CHECK(roundTrip != nullptr && roundTrip->getId() == id);
    }

    // 2. 24종 몬스터 레지스트리 + 약점/스태거 검증
    const auto monIds = MonsterFactory::getRegisteredIds();
    CHECK(monIds.size() == 24);
    for (const auto& id : monIds) {
        auto m = MonsterFactory::createMonster(id);
        CHECK(m != nullptr);
        CHECK(m->getMaxHp() > 0);
        CHECK(m->getMaxStagger() >= 2);
        CHECK(!m->getWeaknesses().empty());
    }

    // 3. 24종 스킬 레지스트리 검증
    const auto skIds = SkillFactory::getRegisteredIds();
    CHECK(skIds.size() == 24);
    for (const auto& id : skIds) {
        CHECK(SkillFactory::createSkill(id) != nullptr);
    }

    // 4. 8종 퀘스트 체인 검증
    const auto qIds = Quest::getCanonicalIds();
    CHECK(qIds.size() == 8);
    for (const auto& id : qIds) {
        auto q = Quest::createCanonical(id);
        CHECK(q != nullptr);
        CHECK(q->matchesCanonicalDefinition());
        CHECK(q->getTargetFloor() >= 1 && q->getTargetFloor() <= 5);
    }

    // 5. 5층 5바이옴 월드 생성 & 도달성·기믹 배치 검증
    DungeonWorld world;
    world.generate(987654321U);
    CHECK(world.getFloorCount() == 5);
    for (int f = 1; f <= 5; ++f) {
        const auto& floor = world.getFloor(f);
        floor.validateForFloor(f);
        CHECK(floor.getBiome() == DungeonMap::biomeForFloor(f));
    }
    int chests = 0, traps = 0, levers = 0, sanctuaries = 0, lore = 0, bosses = 0;
    for (const auto& obj : world.getObjects()) {
        if (obj.kind == WorldObjectKind::CHEST) ++chests;
        if (obj.kind == WorldObjectKind::TRAP) ++traps;
        if (obj.kind == WorldObjectKind::LEVER) ++levers;
        if (obj.kind == WorldObjectKind::SANCTUARY) ++sanctuaries;
        if (obj.kind == WorldObjectKind::LORE_PLAQUE) ++lore;
        if (obj.kind == WorldObjectKind::QUEST_BOSS) ++bosses;
    }
    CHECK(chests >= 10);
    CHECK(traps >= 10);
    CHECK(levers == 5);
    CHECK(sanctuaries == 5);
    CHECK(lore == 5);
    CHECK(bosses == 5);

    // 6. Bestiary JSON 라운드트립
    Bestiary b;
    b.recordEncounter("mon_goblin");
    b.recordDefeat("mon_elder_wyrm");
    const auto b2 = Bestiary::fromJson(b.toJson());
    CHECK(b2.discoveredCount() == 2);
    CHECK(b2.getEntry("mon_elder_wyrm").defeated == 1);
    CHECK(b2.getEntry("mon_elder_wyrm").weaknessRevealed);

    std::cout << "[PASS] ContentContractTests passed.\n";
    return 0;
}
