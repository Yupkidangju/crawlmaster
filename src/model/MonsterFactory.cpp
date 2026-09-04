// [v0.4.0] MonsterFactory.cpp 신규 작성
// [v0.8.0] 신규 몬스터 4종(거대 거미, 구울, 주술사, 새끼 용) 추가 등록 및 스폰 확률 세분화 반영.
// spec.md의 몬스터 기본 기준표를 팩토리에 등록하고 확률 가중치 기반 무작위 몬스터 생성을 구현한다.

#include "model/MonsterFactory.hpp"
#include "core/SessionRng.hpp"
#include <random>

namespace crawl {

std::shared_ptr<Monster> MonsterFactory::createMonster(const std::string& id) {
    if (id == "mon_kobold") {
        return std::make_shared<ConcreteMonster>(
            "mon_kobold", "MONSTER_MON_KOBOLD_NAME", 5, 11, 25, 1, 4, 1
        );
    } else if (id == "mon_goblin") {
        return std::make_shared<ConcreteMonster>(
            "mon_goblin", "MONSTER_MON_GOBLIN_NAME", 7, 12, 50, 1, 6, 1
        );
    } else if (id == "mon_skeleton") {
        return std::make_shared<ConcreteMonster>(
            "mon_skeleton", "MONSTER_MON_SKELETON_NAME", 9, 11, 50, 1, 6, 2
        );
    } else if (id == "mon_giant_spider") {
        // 거대 거미: 25% 독 유발 (전투 컨트롤러에서 id 검출 처리)
        return std::make_shared<ConcreteMonster>(
            "mon_giant_spider", "MONSTER_MON_GIANT_SPIDER_NAME", 12, 12, 75, 1, 6, 1, 2
        );
    } else if (id == "mon_ghoul") {
        // 구울: 20% 마비 유발
        return std::make_shared<ConcreteMonster>(
            "mon_ghoul", "MONSTER_MON_GHOUL_NAME", 18, 12, 120, 1, 8, 2, 3
        );
    } else if (id == "mon_orc") {
        // 오크: 체력 절반 이하 시 분노 버프
        return std::make_shared<ConcreteMonster>(
            "mon_orc", "MONSTER_MON_ORC_NAME", 15, 13, 100, 1, 12, 2, 2
        );
    } else if (id == "mon_goblin_shaman") {
        // 주술사: 35% 매직미사일 시전
        return std::make_shared<ConcreteMonster>(
            "mon_goblin_shaman", "MONSTER_MON_GOBLIN_SHAMAN_NAME", 12, 11, 100, 1, 4, 0, 2
        );
    } else if (id == "mon_dragon_whelp") {
        // 새끼 용: 3턴 광역 화염 브레스
        return std::make_shared<ConcreteMonster>(
            "mon_dragon_whelp", "MONSTER_MON_DRAGON_WHELP_NAME", 35, 14, 250, 1, 8, 3, 4
        );
    } else if (id == "mon_crypt_warden") {
        return std::make_shared<ConcreteMonster>(
            "mon_crypt_warden", "MONSTER_MON_CRYPT_WARDEN_NAME", 28, 14, 200, 1, 8, 3, 3
        );
    }
    return nullptr;
}

std::shared_ptr<Monster> MonsterFactory::createRandomMonster() {
    return createRandomMonster(EncounterTier::MIDDLE, SessionRng::global());
}

std::vector<std::string> MonsterFactory::getEncounterPool(EncounterTier tier) {
    switch (tier) {
        case EncounterTier::EARLY:
            return {"mon_kobold", "mon_goblin", "mon_skeleton"};
        case EncounterTier::MIDDLE:
            return {"mon_goblin", "mon_skeleton", "mon_giant_spider", "mon_orc", "mon_goblin_shaman"};
        case EncounterTier::LATE:
            return {"mon_skeleton", "mon_giant_spider", "mon_orc", "mon_goblin_shaman", "mon_ghoul"};
    }
    return {};
}

std::shared_ptr<Monster> MonsterFactory::createRandomMonster(EncounterTier tier, SessionRng& random) {
    const auto pool = getEncounterPool(tier);
    if (pool.empty()) return nullptr;
    const int index = random.rollRange(0, static_cast<int>(pool.size()) - 1);
    return createMonster(pool[static_cast<std::size_t>(index)]);
}

std::vector<std::string> MonsterFactory::getRegisteredIds() {
    return {
        "mon_kobold", "mon_goblin", "mon_skeleton", "mon_giant_spider",
        "mon_orc", "mon_goblin_shaman", "mon_ghoul", "mon_dragon_whelp", "mon_crypt_warden"
    };
}

std::vector<std::string> MonsterFactory::getDropItemIds(const std::string& monsterId) {
    if (monsterId == "mon_goblin") return {"pot_mana"};
    if (monsterId == "mon_skeleton") return {"wpn_greatsword"};
    if (monsterId == "mon_giant_spider") return {"scr_cure"};
    if (monsterId == "mon_orc") return {"wpn_rapier"};
    if (monsterId == "mon_goblin_shaman") return {"pot_strength", "pot_dexterity"};
    if (monsterId == "mon_ghoul") return {"pot_greater_heal"};
    if (monsterId == "mon_dragon_whelp") return {"arm_plate", "shd_tower"};
    return {};
}

} // namespace crawl
