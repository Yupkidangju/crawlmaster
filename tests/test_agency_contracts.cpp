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
    const std::vector<std::string> names = {"Aster", "Bryn", "Cyra"};
    crawl::SessionRng random(seed);
    crawl::SessionRng mirrorRandom(seed);
    crawl::RecruitmentDraft draft(random, names);
    crawl::RecruitmentDraft mirrorDraft(mirrorRandom, names);
    crawl::Party party;

    CHECK(party.getMemberCount() == 0);
    CHECK(draft.preview().getName() == "Aster");
    CHECK(draft.preview().getName() == mirrorDraft.preview().getName());
    CHECK(draft.preview().getClass() == mirrorDraft.preview().getClass());
    CHECK(party.getMemberCount() == 0);

    draft.reroll();
    mirrorDraft.reroll();
    CHECK(draft.preview().getName() == "Bryn");
    CHECK(draft.preview().getName() == mirrorDraft.preview().getName());
    CHECK(draft.preview().getClass() == mirrorDraft.preview().getClass());
    CHECK(party.getMemberCount() == 0);

    const std::string confirmedName = draft.preview().getName();
    const crawl::CharacterClass confirmedClass = draft.preview().getClass();
    CHECK(draft.confirm(party));
    CHECK(party.getMemberCount() == 1);
    CHECK(party.getMember(0)->getName() == confirmedName);
    CHECK(party.getMember(0)->getClass() == confirmedClass);

    CHECK(!draft.confirm(party));
    CHECK(party.getMemberCount() == 1);
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

} // namespace

int main() {
    testRecruitmentDraftMutatesPartyOnlyOnConfirm();
    testConsumableRulesRejectNoEffectAndAcceptUsefulTargets();
    testCureWoundsUsesOnlyTheExplicitAllyTarget();
    testCureWoundsRejectsFullOrInvalidTargetsWithoutResourceCost();

    if (g_failureCount != 0) {
        std::cerr << "Agency contract tests failed: " << g_failureCount << " check(s).\n";
        return 1;
    }

    std::cout << "Agency contract tests passed.\n";
    return 0;
}
