#ifndef COMBAT_RULES_HPP
#define COMBAT_RULES_HPP

#include "model/Character.hpp"
#include "model/Equipment.hpp"

#include <memory>
#include <vector>

namespace crawl {

class Monster;
class SessionRng;

enum class HitOutcome { MISS, HIT, CRITICAL };

struct AttackResolution {
    int naturalRoll = 1;
    int attackTotal = 0;
    HitOutcome outcome = HitOutcome::MISS;
};

struct TacticalDamageResult {
    int finalDamage = 0;
    bool hitWeakness = false;
    bool hitResistance = false;
    bool triggeredBreak = false;
    int staggerApplied = 0;
    int lifestealHealed = 0;
};

class CombatRules {
public:
    static int initiativeBonus(const Character& character);
    static int spellDamageBonus(const Character& character);
    static int healingBonus(const Character& character);
    static int attackAbilityModifier(const Character& attacker);

    // 진형/사거리 규칙
    static float rowAttackMultiplier(CombatRow row, WeaponReach reach);
    static int rowIncomingAcBonus(CombatRow row, bool isMelee);

    // 속성/스태거/브레이크 규칙
    static float elementalMultiplier(const Monster& target, DamageType type);
    static int computeStaggerDamage(const Monster& target, DamageType type, int baseStagger);

    static AttackResolution resolveAttack(const Character& attacker, int naturalRoll, int targetAc,
                                          int situationalAttackBonus = 0);
    static int rollAttackDamage(int baseDiceCount, int baseDiceSides, int abilityModifier,
                                int flatDamageBonus, int extraDiceCount, int extraDiceSides,
                                bool critical, SessionRng& random);
    static int rollWeaponDamage(const Equipment& weapon, int abilityModifier, SessionRng& random);
    static int mitigateDamage(const Monster& target, DamageType type, int damage);

    // 통합 전술 피해 적용 (약점·저항·브레이크·스태거·생명력 흡수 일괄 계산)
    static TacticalDamageResult applyTacticalDamage(Character& attacker, Monster& target,
                                                    DamageType type, int rawDamage,
                                                    int baseStagger, bool canLifesteal = true);

    static int rollGoldReward(const std::vector<std::shared_ptr<Monster>>& monsters, SessionRng& random);

    // 대장간 강화 / 접사 재연마 공식
    static int blacksmithEnhanceCost(const Equipment& equip);
    static int blacksmithReforgeCost(const Equipment& equip);
    static bool canEnhance(const Equipment& equip);
    static bool enhanceEquipment(Equipment& equip);
    static void reforgeEquipment(Equipment& equip, SessionRng& rng);
};

} // namespace crawl

#endif // COMBAT_RULES_HPP
