#ifndef COMBAT_RULES_HPP
#define COMBAT_RULES_HPP

#include "model/Equipment.hpp"

#include <memory>
#include <vector>

namespace crawl {

class Character;
class Monster;
class SessionRng;

enum class HitOutcome { MISS, HIT, CRITICAL };

struct AttackResolution {
    int naturalRoll;
    int attackTotal;
    HitOutcome outcome;
};

class CombatRules {
public:
    static int initiativeBonus(const Character& character);
    static int spellDamageBonus(const Character& character);
    static int healingBonus(const Character& character);
    static AttackResolution resolveAttack(const Character& attacker, int naturalRoll, int targetAc,
                                          int situationalAttackBonus = 0);
    static int rollAttackDamage(int baseDiceCount, int baseDiceSides, int abilityModifier,
                                int flatDamageBonus, int extraDiceCount, int extraDiceSides,
                                bool critical, SessionRng& random);
    static int rollWeaponDamage(const Equipment& weapon, int abilityModifier, SessionRng& random);
    static int mitigateDamage(const Monster& target, DamageType type, int damage);
    static int rollGoldReward(const std::vector<std::shared_ptr<Monster>>& monsters, SessionRng& random);
};

} // namespace crawl

#endif // COMBAT_RULES_HPP
