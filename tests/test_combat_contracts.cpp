#include "core/SessionRng.hpp"
#include "model/Character.hpp"
#include "model/CombatRules.hpp"
#include "model/ItemFactory.hpp"
#include "model/MonsterFactory.hpp"
#include "model/Skill.hpp"
#include "model/SkillFactory.hpp"

#include <iostream>

#define CHECK(expr)                                                                  \
    do {                                                                             \
        if (!(expr)) {                                                               \
            std::cerr << "[FAIL] CombatContract: " #expr " (" << __LINE__ << ")\n"; \
            return 1;                                                                \
        }                                                                            \
    } while (false)

int main() {
    using namespace crawl;
    SessionRng::global() = SessionRng(424242U, 0);

    // 1. 전열/후열 사거리 및 방어 보정
    CHECK(CombatRules::rowAttackMultiplier(CombatRow::FRONT, WeaponReach::MELEE) == 1.0f);
    CHECK(CombatRules::rowAttackMultiplier(CombatRow::BACK, WeaponReach::MELEE) < 1.0f);
    CHECK(CombatRules::rowAttackMultiplier(CombatRow::BACK, WeaponReach::REACH) == 1.0f);
    CHECK(CombatRules::rowAttackMultiplier(CombatRow::BACK, WeaponReach::RANGED) == 1.0f);
    CHECK(CombatRules::rowIncomingAcBonus(CombatRow::BACK, true) == 2);
    CHECK(CombatRules::rowIncomingAcBonus(CombatRow::FRONT, true) == 0);

    // 2. 속성 약점(1.5x) / 저항(0.5x) & 스태거/브레이크
    Character mage("Lyra", CharacterClass::MAGE);
    auto skel = MonsterFactory::createMonster("mon_skeleton");
    CHECK(skel != nullptr);
    CHECK(skel->isWeakTo(DamageType::HOLY));
    CHECK(skel->isResistantTo(DamageType::PIERCING));
    CHECK(CombatRules::elementalMultiplier(*skel, DamageType::HOLY) == 1.5f);
    CHECK(CombatRules::elementalMultiplier(*skel, DamageType::PIERCING) == 0.5f);

    // 약점 타격으로 스태거 3 감소 -> 브레이크 발동 검증
    const auto res = CombatRules::applyTacticalDamage(mage, *skel, DamageType::HOLY, 10, 1, false);
    CHECK(res.hitWeakness);
    CHECK(res.finalDamage == 15);
    CHECK(res.triggeredBreak);
    CHECK(skel->isBroken());

    // 3. 브레이크 상태 추가 피해(+50%) 및 인텐트 중단 검증
    auto wyrm = MonsterFactory::createMonster("mon_elder_wyrm");
    CHECK(wyrm != nullptr);
    CHECK(wyrm->getPhase() == 1);
    wyrm->takeDamage(wyrm->getMaxHp() / 2 + 5);
    std::vector<std::string> logs;
    CHECK(wyrm->checkPhaseTransition(logs));
    CHECK(wyrm->getPhase() == 2);
    CHECK(wyrm->getNextIntent().kind == EnemyIntentKind::CHARGING);

    // 스태거 모두 깎아 차징 차단
    CHECK(wyrm->applyStagger(wyrm->getMaxStagger()));
    CHECK(wyrm->isBroken());
    CHECK(wyrm->getNextIntent().kind != EnemyIntentKind::CHARGING);

    // 4. 전직 & 특성 트리 & 24종 스킬 검증
    Character warrior("Aldric", CharacterClass::WARRIOR);
    warrior.addXp(600);
    CHECK(warrior.canChooseSpecialization());
    CHECK(warrior.chooseSpecialization(Specialization::BERSERKER));
    bool hasBloodFrenzy = false;
    for (const auto& sk : warrior.getSkills()) {
        if (sk->getId() == "skl_blood_frenzy") hasBloodFrenzy = true;
    }
    CHECK(hasBloodFrenzy);

    // 5. 대장간 강화(+0..+5) 및 재연마 검증
    auto sword = std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("wpn_longsword"));
    CHECK(sword != nullptr);
    const int baseBonus = sword->getUpgradeDamageBonus();
    CHECK(CombatRules::enhanceEquipment(*sword));
    CHECK(sword->getUpgradeLevel() == 1);
    CHECK(sword->getUpgradeDamageBonus() > baseBonus);
    CombatRules::reforgeEquipment(*sword, SessionRng::global());
    CHECK(!sword->getAffix().empty());

    std::cout << "[PASS] CombatContractTests passed.\n";
    return 0;
}
