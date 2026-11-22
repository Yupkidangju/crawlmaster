#include "model/CombatRules.hpp"
#include "core/SessionRng.hpp"
#include "model/ItemFactory.hpp"
#include "model/Monster.hpp"

#include <algorithm>
#include <cmath>

namespace crawl {

int CombatRules::initiativeBonus(const Character& character) {
    const auto ab = character.getAbilities();
    int bonus = ab.getModifier(ab.dexterity);
    if (character.getClass() == CharacterClass::ROGUE) bonus += 2;
    if (character.getSpecialization() == Specialization::ASSASSIN) bonus += 2;
    if (character.getSpecialization() == Specialization::CHRONOMANCER) bonus += 2;
    bonus += character.getTalentRank(TalentNode::TACTICS);
    if (character.getFrostTurns() > 0) bonus -= 2;
    return bonus;
}

int CombatRules::spellDamageBonus(const Character& character) {
    const auto ab = character.getAbilities();
    int bonus = 0;
    if (character.getClass() == CharacterClass::MAGE) {
        bonus = ab.getModifier(ab.intelligence) + 1;
        if (character.getSpecialization() == Specialization::PYROMANCER) bonus += 3;
    } else if (character.getClass() == CharacterClass::CLERIC) {
        bonus = ab.getModifier(ab.wisdom);
        if (character.getSpecialization() == Specialization::PALADIN) bonus += 2;
    } else {
        bonus = std::max(ab.getModifier(ab.intelligence), ab.getModifier(ab.wisdom));
    }
    bonus += character.getTalentRank(TalentNode::MASTERY) * 2;
    if (const auto wpn = character.getEquippedItem(EquipSlot::WEAPON)) {
        bonus += wpn->getUpgradeLevel();
    }
    return bonus;
}

int CombatRules::healingBonus(const Character& character) {
    const auto ab = character.getAbilities();
    int bonus = ab.getModifier(ab.wisdom);
    if (character.getClass() == CharacterClass::CLERIC) bonus += 2;
    if (character.getSpecialization() == Specialization::ORACLE) bonus += 4;
    bonus += character.getTalentRank(TalentNode::APEX) * 2;
    return bonus;
}

int CombatRules::attackAbilityModifier(const Character& attacker) {
    const auto ab = attacker.getAbilities();
    const auto wpn = attacker.getEquippedItem(EquipSlot::WEAPON);
    const bool finesseOrRanged =
        attacker.getClass() == CharacterClass::ROGUE ||
        (wpn && (wpn->getReach() == WeaponReach::RANGED ||
                 wpn->getId() == "wpn_dagger" || wpn->getId() == "wpn_rapier"));
    if (finesseOrRanged) {
        return std::max(ab.getModifier(ab.strength), ab.getModifier(ab.dexterity));
    }
    if (attacker.getClass() == CharacterClass::MAGE && wpn && wpn->getDamageType() == DamageType::ARCANE) {
        return ab.getModifier(ab.intelligence);
    }
    return ab.getModifier(ab.strength);
}

float CombatRules::rowAttackMultiplier(CombatRow row, WeaponReach reach) {
    if (row == CombatRow::BACK && reach == WeaponReach::MELEE) {
        return 0.65f;
    }
    return 1.0f;
}

int CombatRules::rowIncomingAcBonus(CombatRow row, bool isMelee) {
    return (row == CombatRow::BACK && isMelee) ? 2 : 0;
}

float CombatRules::elementalMultiplier(const Monster& target, DamageType type) {
    if (target.isWeakTo(type)) return 1.5f;
    if (target.isResistantTo(type)) return 0.5f;
    return 1.0f;
}

int CombatRules::computeStaggerDamage(const Monster& target, DamageType type, int baseStagger) {
    if (target.isResistantTo(type)) return 0;
    if (target.isWeakTo(type)) return std::max(1, baseStagger) + 2;
    return std::max(0, baseStagger);
}

AttackResolution CombatRules::resolveAttack(const Character& attacker, int naturalRoll,
                                            int targetAc, int situationalAttackBonus) {
    int total = naturalRoll + attackAbilityModifier(attacker) + situationalAttackBonus;
    total += attacker.getTalentRank(TalentNode::MASTERY);
    if (attacker.getBlessTurns() > 0) total += 2;
    if (attacker.getSpecialization() == Specialization::BERSERKER) total += 1;
    if (const auto wpn = attacker.getEquippedItem(EquipSlot::WEAPON)) {
        total += wpn->getUpgradeLevel() / 2;
    }

    if (naturalRoll <= 1) return {naturalRoll, total, HitOutcome::MISS};
    if (naturalRoll >= attacker.getCritThreshold()) return {naturalRoll, total, HitOutcome::CRITICAL};
    return {naturalRoll, total, total >= targetAc ? HitOutcome::HIT : HitOutcome::MISS};
}

int CombatRules::rollAttackDamage(int baseDiceCount, int baseDiceSides, int abilityModifier,
                                  int flatDamageBonus, int extraDiceCount, int extraDiceSides,
                                  bool critical, SessionRng& random) {
    const int rolls = critical ? baseDiceCount * 2 : baseDiceCount;
    const int extraRolls = critical ? extraDiceCount * 2 : extraDiceCount;
    int sum = abilityModifier + flatDamageBonus;
    for (int i = 0; i < rolls; ++i) {
        sum += random.rollDie(std::max(1, baseDiceSides));
    }
    for (int i = 0; i < extraRolls; ++i) {
        sum += random.rollDie(std::max(1, extraDiceSides));
    }
    return std::max(1, sum);
}

int CombatRules::rollWeaponDamage(const Equipment& weapon, int abilityModifier, SessionRng& random) {
    return rollAttackDamage(weapon.getDamageDiceCount(), weapon.getDamageDiceSides(),
                            abilityModifier, weapon.getUpgradeDamageBonus(),
                            0, 0, false, random);
}

int CombatRules::mitigateDamage(const Monster& target, DamageType type, int damage) {
    if (damage <= 0) return 0;
    float mult = elementalMultiplier(target, type);
    if (target.isBroken()) mult *= 1.5f;
    if (target.isDefending()) mult *= 0.6f;
    return std::max(1, static_cast<int>(std::round(static_cast<float>(damage) * mult)));
}

TacticalDamageResult CombatRules::applyTacticalDamage(Character& attacker, Monster& target,
                                                      DamageType type, int rawDamage,
                                                      int baseStagger, bool canLifesteal) {
    TacticalDamageResult result;
    result.hitWeakness = target.isWeakTo(type);
    result.hitResistance = target.isResistantTo(type);

    float mult = elementalMultiplier(target, type);
    if (attacker.getSpecialization() == Specialization::PYROMANCER && type == DamageType::FIRE) {
        mult *= 1.25f;
    }
    if (target.isBroken()) mult *= 1.5f;
    if (target.isDefending()) mult *= 0.6f;

    result.finalDamage = std::max(1, static_cast<int>(std::round(static_cast<float>(rawDamage) * mult)));
    target.takeDamage(result.finalDamage);

    const int staggerDmg = computeStaggerDamage(target, type, baseStagger + attacker.getStaggerBonus());
    result.staggerApplied = staggerDmg;
    if (staggerDmg > 0 && !target.isDead()) {
        result.triggeredBreak = target.applyStagger(staggerDmg);
    }

    if (canLifesteal && !attacker.isDead()) {
        const int pct = attacker.getLifestealPct();
        if (pct > 0) {
            const int healed = std::max(1, result.finalDamage * pct / 100);
            attacker.heal(healed);
            result.lifestealHealed = healed;
        }
    }
    return result;
}

int CombatRules::rollGoldReward(const std::vector<std::shared_ptr<Monster>>& monsters,
                                SessionRng& random) {
    int total = 0;
    for (const auto& mon : monsters) {
        if (!mon) continue;
        const int tier = std::max(1, mon->getTier());
        const int base = tier * 14 + (mon->isBoss() ? 80 * tier : 0);
        total += base + random.rollRange(0, tier * 12);
    }
    return std::max(5, total);
}

int CombatRules::blacksmithEnhanceCost(const Equipment& equip) {
    const int rarityMul = 1 + static_cast<int>(equip.getRarity());
    return 40 * (equip.getUpgradeLevel() + 1) * rarityMul;
}

int CombatRules::blacksmithReforgeCost(const Equipment& equip) {
    const int rarityMul = 1 + static_cast<int>(equip.getRarity());
    return 55 * rarityMul;
}

bool CombatRules::canEnhance(const Equipment& equip) {
    return equip.getUpgradeLevel() < Equipment::MAX_UPGRADE_LEVEL;
}

bool CombatRules::enhanceEquipment(Equipment& equip) {
    if (!canEnhance(equip)) return false;
    equip.setUpgradeLevel(equip.getUpgradeLevel() + 1);
    return true;
}

void CombatRules::reforgeEquipment(Equipment& equip, SessionRng& rng) {
    ItemRarity rar = equip.getRarity();
    if (rar == ItemRarity::COMMON) {
        rar = ItemRarity::UNCOMMON;
        equip.setRarity(rar);
    }
    equip.setAffix(ItemFactory::rollAffix(rar, rng));
}

} // namespace crawl
