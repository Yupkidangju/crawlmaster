#include "model/CombatRules.hpp"

#include "core/SessionRng.hpp"
#include "model/Character.hpp"
#include "model/Monster.hpp"

#include <algorithm>

namespace crawl {

int CombatRules::initiativeBonus(const Character& character) {
    return character.getClass() == CharacterClass::ROGUE ? 2 : 0;
}

int CombatRules::spellDamageBonus(const Character& character) {
    return character.getClass() == CharacterClass::MAGE ? 2 : 0;
}

int CombatRules::healingBonus(const Character& character) {
    return character.getClass() == CharacterClass::CLERIC ? 2 : 0;
}

AttackResolution CombatRules::resolveAttack(const Character& attacker, int naturalRoll, int targetAc,
                                            int situationalAttackBonus) {
    const auto abilities = attacker.getAbilities();
    const int abilityScore = attacker.getClass() == CharacterClass::ROGUE
        ? abilities.dexterity : abilities.strength;
    const int abilityModifier = abilities.getModifier(abilityScore);
    const int blessBonus = attacker.getBlessTurns() > 0 ? 2 : 0;
    const int total = naturalRoll + abilityModifier + 2 + blessBonus + situationalAttackBonus;
    if (naturalRoll == 1) return {naturalRoll, total, HitOutcome::MISS};
    if (naturalRoll == 20) return {naturalRoll, total, HitOutcome::CRITICAL};
    return {naturalRoll, total, total >= targetAc ? HitOutcome::HIT : HitOutcome::MISS};
}

int CombatRules::rollAttackDamage(int baseDiceCount, int baseDiceSides, int abilityModifier,
                                  int flatDamageBonus, int extraDiceCount, int extraDiceSides,
                                  bool critical, SessionRng& random) {
    const int multiplier = critical ? 2 : 1;
    int damage = abilityModifier + flatDamageBonus;
    for (int index = 0; index < baseDiceCount * multiplier; ++index) {
        damage += random.rollDie(baseDiceSides);
    }
    for (int index = 0; index < extraDiceCount * multiplier; ++index) {
        damage += random.rollDie(extraDiceSides);
    }
    return std::max(1, damage);
}

int CombatRules::rollWeaponDamage(const Equipment& weapon, int abilityModifier, SessionRng& random) {
    return rollAttackDamage(weapon.getDamageDiceCount(), weapon.getDamageDiceSides(),
                            abilityModifier, 0, 0, 0, false, random);
}

int CombatRules::mitigateDamage(const Monster& target, DamageType type, int damage) {
    const int nonNegativeDamage = std::max(0, damage);
    if (target.getId() == "mon_skeleton" &&
        (type == DamageType::SLASHING || type == DamageType::PIERCING)) {
        return nonNegativeDamage / 2;
    }
    return nonNegativeDamage;
}

int CombatRules::rollGoldReward(const std::vector<std::shared_ptr<Monster>>& monsters, SessionRng& random) {
    int total = 0;
    for (const auto& monster : monsters) {
        if (monster) total += random.rollDie(10) * monster->getTier();
    }
    return total;
}

} // namespace crawl
