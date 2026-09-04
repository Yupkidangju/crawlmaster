#include "core/SessionRng.hpp"
#include "model/CombatActionRules.hpp"
#include "model/ConcreteItems.hpp"
#include "model/ConcreteSkills.hpp"
#include "model/Party.hpp"
#include "model/RecruitmentDraft.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <utility>
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

std::shared_ptr<crawl::Character> character(const std::string& name,
                                            crawl::CharacterClass characterClass,
                                            int hp,
                                            int maxHp,
                                            int spellSlots,
                                            int maxSpellSlots,
                                            int poisonTurns = 0) {
    const nlohmann::json fixture = {
        {"name", name},
        {"class", static_cast<int>(characterClass)},
        {"level", 1},
        {"xp", 0},
        {"hp", hp},
        {"maxHp", maxHp},
        {"spellSlots", spellSlots},
        {"maxSpellSlots", maxSpellSlots},
        {"poisonTurns", poisonTurns},
        {"paralysisTurns", 0},
        {"abilities", {
            {"strength", 10},
            {"dexterity", 10},
            {"constitution", 10},
            {"intelligence", 10},
            {"wisdom", 10},
            {"charisma", 10},
        }},
        {"equipment", {
            {"weapon", ""},
            {"armor", ""},
            {"shield", ""},
        }},
    };

    return std::shared_ptr<crawl::Character>(crawl::Character::fromJson(fixture));
}

void testRecruitmentDraftMutatesPartyOnlyOnConfirm() {
    constexpr std::uint32_t seed = 0xA63E7U;
    crawl::SessionRng random(seed);
    crawl::SessionRng mirrorRandom(seed);
    crawl::RecruitmentDraft draft(random);
    crawl::RecruitmentDraft mirrorDraft(mirrorRandom);
    crawl::Party party;

    CHECK(party.getMemberCount() == 0);
    CHECK(draft.setName("Aster"));
    CHECK(draft.setAge(27));
    CHECK(draft.setGender(crawl::Gender::NON_BINARY));
    CHECK(draft.setClass(crawl::CharacterClass::ROGUE));
    CHECK(draft.abilities().dexterity == mirrorDraft.abilities().dexterity);
    CHECK(party.getMemberCount() == 0);

    draft.reroll();
    mirrorDraft.reroll();
    CHECK(draft.abilities().strength == mirrorDraft.abilities().strength);
    CHECK(draft.abilities().charisma == mirrorDraft.abilities().charisma);
    CHECK(draft.remainingPoints() == 10);
    CHECK(party.getMemberCount() == 0);

    while (draft.remainingPoints() > 0) {
        bool spent = false;
        for (const auto ability : {crawl::Ability::STRENGTH, crawl::Ability::DEXTERITY,
                                   crawl::Ability::CONSTITUTION, crawl::Ability::INTELLIGENCE,
                                   crawl::Ability::WISDOM, crawl::Ability::CHARISMA}) {
            const int cost = draft.increaseCost(ability);
            if (cost > 0 && cost <= draft.remainingPoints()) {
                CHECK(draft.increase(ability));
                spent = true;
                break;
            }
        }
        if (!spent) {
            draft.reroll();
        }
    }
    CHECK(draft.isReady());
    CHECK(draft.confirm(party));
    CHECK(party.getMemberCount() == 1);
    CHECK(party.getMember(0)->getName() == "Aster");
    CHECK(party.getMember(0)->getAge() == 27);
    CHECK(party.getMember(0)->getGender() == crawl::Gender::NON_BINARY);
    CHECK(party.getMember(0)->getClass() == crawl::CharacterClass::ROGUE);

    CHECK(!draft.confirm(party));
    CHECK(party.getMemberCount() == 1);
}

void testRecruitmentDraftValidatesIdentityAndWeightedPointBuy() {
    crawl::SessionRng random(9001U);
    crawl::RecruitmentDraft draft(random);

    CHECK(!draft.setName(""));
    CHECK(draft.setName(" leading"));
    CHECK(draft.identity().name == "leading");
    CHECK(draft.setName("trailing "));
    CHECK(draft.identity().name == "trailing");
    CHECK(!draft.setName(std::string(17, 'a')));
    CHECK(draft.setName("리아"));
    CHECK(!draft.setAge(17));
    CHECK(!draft.setAge(81));
    CHECK(draft.setAge(18));
    CHECK(!draft.setGender(crawl::Gender::UNSPECIFIED));
    CHECK(!draft.setGender(static_cast<crawl::Gender>(99)));
    CHECK(draft.setGender(crawl::Gender::FEMALE));
    const auto invalidAbility = static_cast<crawl::Ability>(99);
    CHECK(draft.increaseCost(invalidAbility) == 0);
    CHECK(!draft.increase(invalidAbility));
    CHECK(!draft.decrease(invalidAbility));

    const auto base = draft.baseAbilities();
    const auto ability = crawl::Ability::STRENGTH;
    int expectedCost = base.strength < 12 ? 1 : (base.strength < 15 ? 2 : 3);
    if (base.strength < 18) {
        CHECK(draft.increaseCost(ability) == expectedCost);
        CHECK(draft.increase(ability));
        CHECK(draft.remainingPoints() == 10 - expectedCost);
        CHECK(draft.decrease(ability));
        CHECK(draft.remainingPoints() == 10);
    }
    CHECK(!draft.decrease(ability));

    crawl::Party party;
    CHECK(!draft.isReady());
    CHECK(!draft.confirm(party));
    CHECK(party.getMemberCount() == 0);
}

void testConsumableRulesRejectNoEffectAndAcceptUsefulTargets() {
    crawl::HealPotionItem healPotion;
    crawl::GreaterHealPotionItem greaterHealPotion;
    crawl::ManaPotionItem manaPotion;
    crawl::CureScrollItem cureScroll;

    const auto fullWarrior = character("FullWarrior", crawl::CharacterClass::WARRIOR,
                                       10, 10, 0, 0);
    const auto hurtWarrior = character("HurtWarrior", crawl::CharacterClass::WARRIOR,
                                       4, 10, 0, 0);
    const auto fullMage = character("FullMage", crawl::CharacterClass::MAGE,
                                    10, 10, 2, 2);
    const auto spentMage = character("SpentMage", crawl::CharacterClass::MAGE,
                                     10, 10, 1, 2);
    const auto healthyCleric = character("HealthyCleric", crawl::CharacterClass::CLERIC,
                                         10, 10, 2, 2);
    const auto poisonedCleric = character("PoisonedCleric", crawl::CharacterClass::CLERIC,
                                          10, 10, 2, 2, 3);

    CHECK(!crawl::CombatActionRules::canUseConsumable(healPotion, *fullWarrior));
    CHECK(!crawl::CombatActionRules::canUseConsumable(greaterHealPotion, *fullWarrior));
    CHECK(crawl::CombatActionRules::canUseConsumable(healPotion, *hurtWarrior));
    CHECK(crawl::CombatActionRules::canUseConsumable(greaterHealPotion, *hurtWarrior));

    CHECK(!crawl::CombatActionRules::canUseConsumable(manaPotion, *fullMage));
    CHECK(!crawl::CombatActionRules::canUseConsumable(manaPotion, *fullWarrior));
    CHECK(crawl::CombatActionRules::canUseConsumable(manaPotion, *spentMage));

    CHECK(!crawl::CombatActionRules::canUseConsumable(cureScroll, *healthyCleric));
    CHECK(crawl::CombatActionRules::canUseConsumable(cureScroll, *poisonedCleric));
}

void testCureWoundsUsesOnlyTheExplicitAllyTarget() {
    auto caster = character("Cleric", crawl::CharacterClass::CLERIC, 10, 10, 2, 2);
    auto lowestHpAlly = character("Lowest", crawl::CharacterClass::WARRIOR, 1, 10, 0, 0);
    auto selectedAlly = character("Selected", crawl::CharacterClass::ROGUE, 8, 10, 0, 0);
    std::vector<std::shared_ptr<crawl::Character>> allies = {lowestHpAlly, selectedAlly};
    std::vector<std::shared_ptr<crawl::Monster>> foes;
    std::vector<std::string> logs;
    crawl::CureWoundsSpell cureWounds;

    const int casterSlotsBefore = caster->getSpellSlots();
    const int lowestHpBefore = lowestHpAlly->getHp();
    const int selectedHpBefore = selectedAlly->getHp();
    const bool consumedTurn = cureWounds.execute(*caster, allies, foes, 1, logs);

    CHECK(consumedTurn);
    CHECK(caster->getSpellSlots() == casterSlotsBefore - 1);
    CHECK(lowestHpAlly->getHp() == lowestHpBefore);
    CHECK(selectedAlly->getHp() > selectedHpBefore);
}

void testCureWoundsRejectsFullOrInvalidTargetsWithoutResourceCost() {
    auto caster = character("Cleric", crawl::CharacterClass::CLERIC, 10, 10, 2, 2);
    auto fullHpAlly = character("Full", crawl::CharacterClass::WARRIOR, 10, 10, 0, 0);
    auto hurtAlly = character("Hurt", crawl::CharacterClass::ROGUE, 5, 10, 0, 0);
    std::vector<std::shared_ptr<crawl::Character>> allies = {fullHpAlly, hurtAlly};
    std::vector<std::shared_ptr<crawl::Monster>> foes;
    std::vector<std::string> logs;
    crawl::CureWoundsSpell cureWounds;

    const int initialSlots = caster->getSpellSlots();
    CHECK(!cureWounds.execute(*caster, allies, foes, 0, logs));
    CHECK(caster->getSpellSlots() == initialSlots);
    CHECK(fullHpAlly->getHp() == fullHpAlly->getMaxHp());

    const int hurtHpBefore = hurtAlly->getHp();
    CHECK(!cureWounds.execute(*caster, allies, foes, -1, logs));
    CHECK(!cureWounds.execute(*caster, allies, foes, static_cast<int>(allies.size()), logs));
    CHECK(caster->getSpellSlots() == initialSlots);
    CHECK(hurtAlly->getHp() == hurtHpBefore);
}

nlohmann::json canonicalCharacterJson(const std::string& name, crawl::CharacterClass characterClass,
                                      int level, int spellSlots, int maxSpellSlots) {
    return {
        {"name", name}, {"age", 24}, {"gender", "non_binary"},
        {"class", static_cast<int>(characterClass)}, {"level", level}, {"xp", 0},
        {"hp", 10}, {"maxHp", 10}, {"spellSlots", spellSlots},
        {"maxSpellSlots", maxSpellSlots}, {"poisonTurns", 0}, {"paralysisTurns", 0},
        {"abilities", {{"strength", 10}, {"dexterity", 10}, {"constitution", 10},
                       {"intelligence", 10}, {"wisdom", 10}, {"charisma", 10}}},
        {"equipment", {{"weapon", ""}, {"armor", ""}, {"shield", ""}}},
    };
}

void testSharedIdentityRulesRejectMalformedAndNonCanonicalNames() {
    crawl::SessionRng random(123U);
    crawl::RecruitmentDraft draft(random);
    CHECK(draft.setName("\xE3\x80\x80" "Aster" "\xE3\x80\x80"));
    CHECK(draft.identity().name == "Aster");
    CHECK(!crawl::RecruitmentDraft::isValidName(std::string(17, 'a')));
    CHECK(!crawl::RecruitmentDraft::isValidName(std::string("\xC3\x28", 2)));
    CHECK(!crawl::RecruitmentDraft::isValidName(std::string("A\xC2\x85", 3)));
    CHECK(!crawl::RecruitmentDraft::isValidName(std::string("A\xC2\xAD", 3)));
    CHECK(!crawl::RecruitmentDraft::isValidName(std::string("A\xD8\x9C", 3)));
    CHECK(!crawl::RecruitmentDraft::isValidName(std::string("A\xE2\x80\x8B", 4)));

    const std::vector<std::string> invalidPersistedNames = {
        " Aster", std::string(17, 'a'), std::string("\xC3\x28", 2),
        std::string("A\xC2\x85", 3), std::string("A\xE2\x80\x8B", 4)};
    for (const auto& name : invalidPersistedNames) {
        bool rejected = false;
        try {
            static_cast<void>(crawl::Character::fromJson(
                canonicalCharacterJson(name, crawl::CharacterClass::WARRIOR, 1, 0, 0), 4));
        } catch (const std::exception&) {
            rejected = true;
        }
        CHECK(rejected);
    }
}

void testSpellSlotsMatchClassAndLevel() {
    const std::vector<crawl::CharacterClass> classes = {
        crawl::CharacterClass::WARRIOR, crawl::CharacterClass::MAGE,
        crawl::CharacterClass::ROGUE, crawl::CharacterClass::CLERIC};
    for (const auto characterClass : classes) {
        for (int level = 1; level <= 3; ++level) {
            const bool caster = characterClass == crawl::CharacterClass::MAGE ||
                                characterClass == crawl::CharacterClass::CLERIC;
            const int expected = caster ? level + 1 : 0;
            CHECK(crawl::Character::fromJson(
                canonicalCharacterJson("Valid", characterClass, level, expected, expected), 4) != nullptr);
            bool rejected = false;
            try {
                static_cast<void>(crawl::Character::fromJson(
                    canonicalCharacterJson("Invalid", characterClass, level, 0, expected + 1), 4));
            } catch (const std::exception&) {
                rejected = true;
            }
            CHECK(rejected);
        }
    }
}

void testV4CharacterFieldsAreRequired() {
    const auto base = canonicalCharacterJson("Required", crawl::CharacterClass::CLERIC, 1, 2, 2);
    const std::vector<std::string> required = {
        "name", "age", "gender", "class", "level", "xp", "hp", "maxHp",
        "spellSlots", "maxSpellSlots", "poisonTurns", "paralysisTurns", "abilities", "equipment"};
    for (const auto& key : required) {
        auto malformed = base;
        malformed.erase(key);
        bool rejected = false;
        try { static_cast<void>(crawl::Character::fromJson(malformed, 4)); }
        catch (const std::exception&) { rejected = true; }
        CHECK(rejected);
    }
}

} // namespace

int main() {
    testRecruitmentDraftMutatesPartyOnlyOnConfirm();
    testRecruitmentDraftValidatesIdentityAndWeightedPointBuy();
    testConsumableRulesRejectNoEffectAndAcceptUsefulTargets();
    testCureWoundsUsesOnlyTheExplicitAllyTarget();
    testCureWoundsRejectsFullOrInvalidTargetsWithoutResourceCost();
    testSharedIdentityRulesRejectMalformedAndNonCanonicalNames();
    testSpellSlotsMatchClassAndLevel();
    testV4CharacterFieldsAreRequired();

    if (g_failureCount != 0) {
        std::cerr << "Agency contract tests failed: " << g_failureCount << " check(s).\n";
        return 1;
    }

    std::cout << "Agency contract tests passed.\n";
    return 0;
}
