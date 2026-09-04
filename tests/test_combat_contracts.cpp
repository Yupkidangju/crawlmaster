#include "core/SessionRng.hpp"
#include "model/Character.hpp"
#include "model/CombatRules.hpp"
#include "model/ItemFactory.hpp"
#include "model/MonsterFactory.hpp"
#include "model/SkillFactory.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
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

std::shared_ptr<crawl::Equipment> equipment(const std::string& id) {
    return std::dynamic_pointer_cast<crawl::Equipment>(crawl::ItemFactory::createItem(id));
}

std::unique_ptr<crawl::Character> characterWithStrength(crawl::CharacterClass characterClass,
                                                        int strength,
                                                        int dexterity = 10) {
    const bool spellcaster = characterClass == crawl::CharacterClass::MAGE ||
                             characterClass == crawl::CharacterClass::CLERIC;
    const nlohmann::json fixture = {
        {"name", "ContractHero"},
        {"class", static_cast<int>(characterClass)},
        {"level", 1},
        {"xp", 0},
        {"hp", 10},
        {"maxHp", 10},
        {"spellSlots", spellcaster ? 2 : 0},
        {"maxSpellSlots", spellcaster ? 2 : 0},
        {"poisonTurns", 0},
        {"paralysisTurns", 0},
        {"abilities", {
            {"strength", strength},
            {"dexterity", dexterity},
            {"constitution", 10},
            {"intelligence", 10},
            {"wisdom", 10},
            {"charisma", 10}
        }},
        {"equipment", {
            {"weapon", ""},
            {"armor", ""},
            {"shield", ""}
        }}
    };
    return crawl::Character::fromJson(fixture);
}

std::shared_ptr<crawl::Character> spellcaster(crawl::CharacterClass characterClass) {
    const bool usesSpellSlots = characterClass == crawl::CharacterClass::MAGE ||
                               characterClass == crawl::CharacterClass::CLERIC;
    const nlohmann::json fixture = {
        {"name", "Caster"}, {"class", static_cast<int>(characterClass)},
        {"level", 1}, {"xp", 0}, {"hp", 20}, {"maxHp", 20},
        {"spellSlots", usesSpellSlots ? 2 : 0}, {"maxSpellSlots", usesSpellSlots ? 2 : 0},
        {"poisonTurns", 0}, {"paralysisTurns", 0},
        {"abilities", {{"strength", 10}, {"dexterity", 10}, {"constitution", 10},
                       {"intelligence", 10}, {"wisdom", 10}, {"charisma", 10}}},
        {"equipment", {{"weapon", ""}, {"armor", ""}, {"shield", ""}}}
    };
    return std::shared_ptr<crawl::Character>(crawl::Character::fromJson(fixture));
}

int rollExpectedDice(crawl::SessionRng& random, int count, int sides) {
    int total = 0;
    for (int index = 0; index < count; ++index) total += random.rollDie(sides);
    return total;
}

std::shared_ptr<crawl::Monster> contractSkeleton(int ac = 1, int hp = 200) {
    return std::make_shared<crawl::ConcreteMonster>(
        "mon_skeleton", "ContractSkeleton", hp, ac, 0, 1, 4, 0, 1);
}

void testProductionWeaponSkillsUseCanonicalAttackAndDamageRules() {
    struct Case {
        const char* skillId;
        crawl::CharacterClass characterClass;
        const char* weaponId;
        int skillAttackBonus;
        int flatDamageBonus;
        int extraDiceCount;
        int extraDiceSides;
        std::uint32_t seed;
    };
    const std::array<Case, 4> cases = {{
        {"skl_slash", crawl::CharacterClass::WARRIOR, "wpn_greatsword", 4, 2, 0, 0, 7001U},
        {"skl_cleave", crawl::CharacterClass::WARRIOR, "wpn_greatsword", 0, 0, 0, 0, 7002U},
        {"skl_sneak_attack", crawl::CharacterClass::ROGUE, "wpn_rapier", 2, 0, 2, 4, 7003U},
        {"skl_shadowstrike", crawl::CharacterClass::ROGUE, "wpn_rapier", 1, 0, 3, 4, 7004U}
    }};

    for (const auto& testCase : cases) {
        auto attacker = characterWithStrength(testCase.characterClass, 10, 10);
        const auto weapon = equipment(testCase.weaponId);
        CHECK(attacker->equip(weapon));
        attacker->applyBless(3);

        crawl::SessionRng expected(testCase.seed);
        const int naturalRoll = expected.rollDie(20);
        const int targetAc = naturalRoll + 2 + testCase.skillAttackBonus + 1;
        auto target = contractSkeleton(targetAc);
        std::vector<std::shared_ptr<crawl::Monster>> foes{target};
        std::vector<std::shared_ptr<crawl::Character>> allies;
        std::vector<std::string> logs;
        const auto skill = crawl::SkillFactory::createSkill(testCase.skillId);
        CHECK(skill != nullptr);

        const int multiplier = naturalRoll == 20 ? 2 : 1;
        int rawDamage = rollExpectedDice(expected,
            weapon->getDamageDiceCount() * multiplier, weapon->getDamageDiceSides());
        rawDamage += rollExpectedDice(expected,
            testCase.extraDiceCount * multiplier, testCase.extraDiceSides);
        rawDamage = std::max(1, rawDamage + testCase.flatDamageBonus);
        const int expectedDamage = crawl::CombatRules::mitigateDamage(
            *target, weapon->getDamageType(), rawDamage);

        crawl::SessionRng::reseedGlobal(testCase.seed);
        const int hpBefore = target->getHp();
        CHECK(skill->execute(*attacker, allies, foes, 0, logs));
        CHECK(hpBefore - target->getHp() == expectedDamage);
    }
}

void testSessionRngIsSeedReproducible() {
    constexpr std::uint32_t seed = 0x00C0FFEEU;
    crawl::SessionRng first(seed);
    crawl::SessionRng second(seed);
    crawl::SessionRng different(seed + 1U);

    CHECK(first.seed() == seed);
    CHECK(second.seed() == seed);

    bool differentSeedProducedDifferentValue = false;
    for (int i = 0; i < 24; ++i) {
        const int firstValue = first.rollDie(1'000'000);
        const int secondValue = second.rollDie(1'000'000);
        const int differentValue = different.rollDie(1'000'000);
        CHECK(firstValue == secondValue);
        differentSeedProducedDifferentValue |= firstValue != differentValue;
    }
    CHECK(differentSeedProducedDifferentValue);
}

void testWeaponDiceCountAndAttackContracts() {
    const auto greatsword = equipment("wpn_greatsword");
    CHECK(greatsword != nullptr);
    CHECK(greatsword->getDamageDiceCount() == 2);
    CHECK(greatsword->getDamageDiceSides() == 6);

    constexpr std::uint32_t damageSeed = 4105U;
    crawl::SessionRng expectedDamageRng(damageSeed);
    crawl::SessionRng actualDamageRng(damageSeed);
    const int expectedDamage = expectedDamageRng.rollDie(6) + expectedDamageRng.rollDie(6);
    CHECK(crawl::CombatRules::rollWeaponDamage(*greatsword, 0, actualDamageRng) == expectedDamage);

    auto attacker = characterWithStrength(crawl::CharacterClass::WARRIOR, 10);
    const auto unblessed = crawl::CombatRules::resolveAttack(*attacker, 9, 12);
    CHECK(unblessed.attackTotal == 11);
    CHECK(unblessed.outcome == crawl::HitOutcome::MISS);

    attacker->applyBless(3);
    const auto blessed = crawl::CombatRules::resolveAttack(*attacker, 9, 12);
    CHECK(blessed.attackTotal == 13);
    CHECK(blessed.outcome == crawl::HitOutcome::HIT);

    auto strongestAttacker = characterWithStrength(crawl::CharacterClass::WARRIOR, 30);
    const auto naturalOne = crawl::CombatRules::resolveAttack(*strongestAttacker, 1, 1);
    CHECK(naturalOne.outcome == crawl::HitOutcome::MISS);

    auto weakestAttacker = characterWithStrength(crawl::CharacterClass::WARRIOR, 1);
    const auto naturalTwenty = crawl::CombatRules::resolveAttack(*weakestAttacker, 20, 999);
    CHECK(naturalTwenty.outcome == crawl::HitOutcome::CRITICAL);
}

void testSkeletonDamageTypeMitigation() {
    const auto skeleton = crawl::MonsterFactory::createMonster("mon_skeleton");
    const auto longsword = equipment("wpn_longsword");
    const auto rapier = equipment("wpn_rapier");
    const auto mace = equipment("wpn_mace");
    CHECK(skeleton != nullptr);
    CHECK(longsword != nullptr);
    CHECK(rapier != nullptr);
    CHECK(mace != nullptr);
    CHECK(longsword->getDamageType() == crawl::DamageType::SLASHING);
    CHECK(rapier->getDamageType() == crawl::DamageType::PIERCING);
    CHECK(mace->getDamageType() == crawl::DamageType::BLUDGEONING);

    constexpr int incomingDamage = 8;
    const int slashingDamage = crawl::CombatRules::mitigateDamage(
        *skeleton, longsword->getDamageType(), incomingDamage);
    const int piercingDamage = crawl::CombatRules::mitigateDamage(
        *skeleton, rapier->getDamageType(), incomingDamage);
    const int bludgeoningDamage = crawl::CombatRules::mitigateDamage(
        *skeleton, mace->getDamageType(), incomingDamage);

    CHECK(slashingDamage >= 0);
    CHECK(slashingDamage < incomingDamage);
    CHECK(piercingDamage >= 0);
    CHECK(piercingDamage < incomingDamage);
    CHECK(bludgeoningDamage == incomingDamage);
}

void testEquipmentEligibilityAndTwoHandedInvariant() {
    const auto longsword = equipment("wpn_longsword");
    const auto greatsword = equipment("wpn_greatsword");
    const auto plate = equipment("arm_plate");
    const auto roundShield = equipment("shd_round");
    const auto towerShield = equipment("shd_tower");
    CHECK(longsword && greatsword && plate && roundShield && towerShield);

    auto mage = characterWithStrength(crawl::CharacterClass::MAGE, 18);
    auto rogue = characterWithStrength(crawl::CharacterClass::ROGUE, 18);
    auto cleric = characterWithStrength(crawl::CharacterClass::CLERIC, 18);
    CHECK(!mage->canEquip(*longsword));
    CHECK(!mage->equip(longsword));
    CHECK(!rogue->canEquip(*longsword));
    CHECK(!cleric->canEquip(*greatsword));
    CHECK(!cleric->canEquip(*plate));
    CHECK(cleric->canEquip(*longsword));

    auto strength14 = characterWithStrength(crawl::CharacterClass::WARRIOR, 14);
    auto strength15 = characterWithStrength(crawl::CharacterClass::WARRIOR, 15);
    CHECK(!strength14->canEquip(*plate));
    CHECK(!strength14->equip(plate));
    CHECK(strength15->canEquip(*plate));
    CHECK(strength15->equip(plate));

    auto strength13 = characterWithStrength(crawl::CharacterClass::WARRIOR, 13);
    CHECK(!strength13->canEquip(*towerShield));
    CHECK(!strength13->equip(towerShield));
    CHECK(strength14->canEquip(*towerShield));
    CHECK(strength14->equip(towerShield));

    auto greatswordFirst = characterWithStrength(crawl::CharacterClass::WARRIOR, 18);
    CHECK(greatswordFirst->equip(greatsword));
    CHECK(!greatswordFirst->canEquip(*roundShield));
    CHECK(!greatswordFirst->equip(roundShield));
    CHECK(greatswordFirst->getEquippedItem(crawl::EquipSlot::WEAPON)->getId() ==
          "wpn_greatsword");
    CHECK(greatswordFirst->getEquippedItem(crawl::EquipSlot::SHIELD) == nullptr);

    auto shieldFirst = characterWithStrength(crawl::CharacterClass::WARRIOR, 18);
    CHECK(shieldFirst->equip(roundShield));
    CHECK(!shieldFirst->canEquip(*greatsword));
    CHECK(!shieldFirst->equip(greatsword));
    CHECK(shieldFirst->getEquippedItem(crawl::EquipSlot::WEAPON) == nullptr);
    CHECK(shieldFirst->getEquippedItem(crawl::EquipSlot::SHIELD)->getId() == "shd_round");
}

void testRewardUsesOneD10PerMonsterTier() {
    const std::vector<std::shared_ptr<crawl::Monster>> defeated = {
        crawl::MonsterFactory::createMonster("mon_kobold"),
        crawl::MonsterFactory::createMonster("mon_skeleton"),
        crawl::MonsterFactory::createMonster("mon_ghoul")
    };
    CHECK(std::all_of(defeated.begin(), defeated.end(), [](const auto& monster) {
        return monster != nullptr;
    }));
    CHECK(defeated[0]->getTier() == 1);
    CHECK(defeated[1]->getTier() == 1);
    CHECK(defeated[2]->getTier() == 3);

    constexpr std::uint32_t rewardSeed = 9901U;
    crawl::SessionRng expectedRewardRng(rewardSeed);
    crawl::SessionRng actualRewardRng(rewardSeed);
    int expectedGold = 0;
    for (const auto& monster : defeated) {
        expectedGold += expectedRewardRng.rollDie(10) * monster->getTier();
    }
    CHECK(crawl::CombatRules::rollGoldReward(defeated, actualRewardRng) == expectedGold);
}

void testEncounterTiersExcludeBossAndRestrictEarlyPool() {
    const auto early = crawl::MonsterFactory::getEncounterPool(crawl::EncounterTier::EARLY);
    const auto middle = crawl::MonsterFactory::getEncounterPool(crawl::EncounterTier::MIDDLE);
    const auto late = crawl::MonsterFactory::getEncounterPool(crawl::EncounterTier::LATE);
    const std::array<std::vector<std::string>, 3> pools = {early, middle, late};

    CHECK(!early.empty());
    CHECK(!middle.empty());
    CHECK(!late.empty());
    for (const auto& pool : pools) {
        CHECK(std::find(pool.begin(), pool.end(), "mon_dragon_whelp") == pool.end());
    }

    const std::array<std::string, 3> allowedEarly = {
        "mon_kobold", "mon_goblin", "mon_skeleton"
    };
    for (const auto& id : early) {
        CHECK(std::find(allowedEarly.begin(), allowedEarly.end(), id) != allowedEarly.end());
    }

    crawl::SessionRng first(314159U);
    crawl::SessionRng second(314159U);
    for (int i = 0; i < 64; ++i) {
        const auto firstMonster = crawl::MonsterFactory::createRandomMonster(
            crawl::EncounterTier::EARLY, first);
        const auto secondMonster = crawl::MonsterFactory::createRandomMonster(
            crawl::EncounterTier::EARLY, second);
        CHECK(firstMonster != nullptr);
        CHECK(secondMonster != nullptr);
        CHECK(firstMonster->getId() == secondMonster->getId());
        CHECK(firstMonster->getId() != "mon_dragon_whelp");
        CHECK(std::find(allowedEarly.begin(), allowedEarly.end(), firstMonster->getId()) !=
              allowedEarly.end());
    }
}

void testLevelOneClassTraitsUseCanonicalRules() {
    auto warrior = characterWithStrength(crawl::CharacterClass::WARRIOR, 14, 10);
    CHECK(warrior->equip(equipment("arm_scale")));
    CHECK(warrior->getAc() == 15);

    const auto rogue = characterWithStrength(crawl::CharacterClass::ROGUE, 10, 16);
    const auto mage = characterWithStrength(crawl::CharacterClass::MAGE, 10, 10);
    const auto cleric = characterWithStrength(crawl::CharacterClass::CLERIC, 10, 10);
    CHECK(crawl::CombatRules::initiativeBonus(*rogue) == 2);
    CHECK(crawl::CombatRules::initiativeBonus(*warrior) == 0);
    CHECK(crawl::CombatRules::spellDamageBonus(*mage) == 2);
    CHECK(crawl::CombatRules::spellDamageBonus(*cleric) == 0);
    CHECK(crawl::CombatRules::healingBonus(*cleric) == 2);
    CHECK(crawl::CombatRules::healingBonus(*mage) == 0);
}

void testClassTraitBonusesReachSpellEffects() {
    auto mage = spellcaster(crawl::CharacterClass::MAGE);
    auto clericUsingArcaneSpell = spellcaster(crawl::CharacterClass::CLERIC);
    auto mageTarget = contractSkeleton(1, 200);
    auto clericTarget = contractSkeleton(1, 200);
    std::vector<std::shared_ptr<crawl::Character>> noAllies;
    std::vector<std::shared_ptr<crawl::Monster>> mageFoes{mageTarget};
    std::vector<std::string> logs;
    auto missile = crawl::SkillFactory::createSkill("spl_magic_missile");
    crawl::SessionRng::reseedGlobal(4401U);
    CHECK(missile->execute(*mage, noAllies, mageFoes, 0, logs));
    logs.clear();
    std::vector<std::shared_ptr<crawl::Monster>> clericFoes{clericTarget};
    crawl::SessionRng::reseedGlobal(4401U);
    CHECK(missile->execute(*clericUsingArcaneSpell, noAllies, clericFoes, 0, logs));
    CHECK(clericTarget->getHp() - mageTarget->getHp() == 2);

    auto healingCleric = spellcaster(crawl::CharacterClass::CLERIC);
    auto healingMage = spellcaster(crawl::CharacterClass::MAGE);
    auto clericAlly = spellcaster(crawl::CharacterClass::WARRIOR);
    auto mageAlly = spellcaster(crawl::CharacterClass::WARRIOR);
    clericAlly->takeDamage(15);
    mageAlly->takeDamage(15);
    std::vector<std::shared_ptr<crawl::Character>> clericAllies{clericAlly};
    std::vector<std::shared_ptr<crawl::Character>> mageAllies{mageAlly};
    std::vector<std::shared_ptr<crawl::Monster>> noFoes;
    auto cure = crawl::SkillFactory::createSkill("spl_cure_wounds");
    crawl::SessionRng::reseedGlobal(4402U);
    CHECK(cure->execute(*healingCleric, clericAllies, noFoes, 0, logs));
    logs.clear();
    crawl::SessionRng::reseedGlobal(4402U);
    CHECK(cure->execute(*healingMage, mageAllies, noFoes, 0, logs));
    CHECK(clericAlly->getHp() - mageAlly->getHp() == 2);
}

} // namespace

int main() {
    testSessionRngIsSeedReproducible();
    testProductionWeaponSkillsUseCanonicalAttackAndDamageRules();
    testWeaponDiceCountAndAttackContracts();
    testSkeletonDamageTypeMitigation();
    testEquipmentEligibilityAndTwoHandedInvariant();
    testRewardUsesOneD10PerMonsterTier();
    testEncounterTiersExcludeBossAndRestrictEarlyPool();
    testLevelOneClassTraitsUseCanonicalRules();
    testClassTraitBonusesReachSpellEffects();

    if (g_failureCount != 0) {
        std::cerr << "[Result] " << g_failureCount << " combat contract check(s) failed.\n";
        return 1;
    }
    std::cout << "[Result] All combat contracts passed.\n";
    return 0;
}
