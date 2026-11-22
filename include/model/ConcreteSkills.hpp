#ifndef CONCRETE_SKILLS_HPP
#define CONCRETE_SKILLS_HPP

#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/Character.hpp"
#include "model/CombatRules.hpp"
#include "model/Monster.hpp"
#include "model/Skill.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace crawl {

// 데이터 드리븐 통합 전술 스킬/주문 구현:
// 24종 직업/전직 스킬이 모두 CombatRules(약점·스태거·브레이크·진형·치명타·생명력흡수)를 공유한다.
struct SkillSpec {
    std::string id;
    int reqLevel = 1;
    bool isSpell = false;
    int spellLevel = 0;
    SkillTargetType target = SkillTargetType::SINGLE_FOE;
    DamageType damageType = DamageType::SLASHING;
    bool usesWeapon = false;
    bool guaranteedHit = false;
    bool requiresShield = false;
    int attackBonus = 0;
    int diceCount = 1;
    int diceSides = 6;
    int flatBonus = 0;
    int baseStagger = 1;
    // 효과 플래그
    bool isHeal = false;
    bool isRevive = false;
    bool curesDebuffs = false;
    bool appliesBless = false;
    bool appliesWard = false;
    bool appliesTaunt = false;
    bool appliesStrBuff = false;
    bool appliesDexBuff = false;
    bool inflictsPoison = false;
    bool inflictsBurn = false;
    bool inflictsBleed = false;
    bool inflictsFrost = false;
    bool inflictsParalysis = false;
    bool bonusVsBrokenOrLowHp = false;
    bool extraLifesteal = false;
};

class TacticalSkill : public Skill {
public:
    explicit TacticalSkill(SkillSpec spec) : m_spec(std::move(spec)) {}

    [[nodiscard]] std::string getId() const override { return m_spec.id; }
    [[nodiscard]] int getRequiredLevel() const override { return m_spec.reqLevel; }
    [[nodiscard]] bool isSpell() const override { return m_spec.isSpell; }
    [[nodiscard]] int getSpellLevel() const override { return m_spec.spellLevel; }
    [[nodiscard]] SkillTargetType getTargetType() const override { return m_spec.target; }
    [[nodiscard]] DamageType getDamageType() const override { return m_spec.damageType; }
    [[nodiscard]] int getBaseStagger() const override { return m_spec.baseStagger; }

    bool execute(Character& caster,
                 std::vector<std::shared_ptr<Character>>& allies,
                 std::vector<std::shared_ptr<Monster>>& foes,
                 int targetIdx,
                 std::vector<std::string>& logOutput) override {
        const auto& lm = LocalizationManager::getInstance();
        auto& rng = SessionRng::global();

        if (m_spec.requiresShield && !caster.getEquippedItem(EquipSlot::SHIELD)) {
            logOutput.push_back(lm.get("SKILL_LOG_SHIELD_REQUIRED"));
            return false;
        }
        if (m_spec.isSpell && caster.getSpellSlots() <= 0) {
            logOutput.push_back(lm.get("SKILL_LOG_NO_SLOTS"));
            return false;
        }

        // === 1. 치유 / 부활 / 버프형 스킬 ===
        if (m_spec.isHeal || m_spec.isRevive || m_spec.appliesBless || m_spec.appliesWard ||
            m_spec.appliesTaunt || m_spec.appliesStrBuff || m_spec.appliesDexBuff) {
            if (m_spec.target == SkillTargetType::SINGLE_ALLY) {
                if (targetIdx < 0 || targetIdx >= static_cast<int>(allies.size()) || !allies[static_cast<std::size_t>(targetIdx)]) {
                    return false;
                }
                auto& ally = *allies[static_cast<std::size_t>(targetIdx)];
                if (m_spec.isRevive) {
                    if (!ally.isDead() && ally.getHp() >= ally.getMaxHp() && !ally.hasAnyDebuff()) {
                        logOutput.push_back(lm.get("COMBAT_LOG_ITEM_NO_EFFECT"));
                        return false;
                    }
                    const int amt = std::max(10, ally.getMaxHp() * 65 / 100 + CombatRules::healingBonus(caster));
                    if (ally.isDead()) ally.revive(amt);
                    else ally.heal(amt);
                    ally.cureAllDebuffs();
                    logOutput.push_back(lm.format("SKILL_LOG_REVIVE", {
                        {"caster", caster.getName()}, {"target", ally.getName()}, {"amount", std::to_string(amt)}}));
                } else {
                    if (ally.isDead()) return false;
                    if (m_spec.isHeal && ally.getHp() >= ally.getMaxHp() && (!m_spec.curesDebuffs || !ally.hasAnyDebuff())) {
                        logOutput.push_back(lm.get("COMBAT_LOG_ITEM_NO_EFFECT"));
                        return false;
                    }
                    int total = m_spec.flatBonus + CombatRules::healingBonus(caster);
                    for (int i = 0; i < m_spec.diceCount; ++i) total += rng.rollDie(m_spec.diceSides);
                    total = std::max(2, total);
                    ally.heal(total);
                    if (m_spec.curesDebuffs) ally.cureAllDebuffs();
                    logOutput.push_back(lm.format("SKILL_LOG_HEAL", {
                        {"caster", caster.getName()}, {"skill", getName()},
                        {"target", ally.getName()}, {"amount", std::to_string(total)}}));
                }
                if (m_spec.isSpell) caster.consumeSpellSlot();
                return true;
            }

            if (m_spec.target == SkillTargetType::ALL_ALLIES || m_spec.target == SkillTargetType::SELF) {
                if (m_spec.isHeal) {
                    bool anyNeeds = false;
                    for (const auto& a : allies) {
                        if (a && !a->isDead() && (a->getHp() < a->getMaxHp() || (m_spec.curesDebuffs && a->hasAnyDebuff()))) {
                            anyNeeds = true;
                        }
                    }
                    if (!anyNeeds) {
                        logOutput.push_back(lm.get("COMBAT_LOG_ITEM_NO_EFFECT"));
                        return false;
                    }
                }

                for (auto& a : allies) {
                    if (!a || a->isDead()) continue;
                    if (m_spec.target == SkillTargetType::SELF && a.get() != &caster) continue;
                    if (m_spec.isHeal) {
                        int total = m_spec.flatBonus + CombatRules::healingBonus(caster);
                        for (int i = 0; i < m_spec.diceCount; ++i) total += rng.rollDie(m_spec.diceSides);
                        a->heal(std::max(2, total));
                        if (m_spec.curesDebuffs) a->cureAllDebuffs();
                    }
                    if (m_spec.appliesBless) a->applyBless(4);
                    if (m_spec.appliesWard) a->applyWard(3);
                    if (m_spec.appliesStrBuff) a->applyStrBuff(3, 3);
                    if (m_spec.appliesDexBuff) a->applyDexBuff(3, 3);
                }
                if (m_spec.appliesTaunt) {
                    caster.setTaunt(3);
                    caster.setGuarding(true);
                }
                logOutput.push_back(lm.format("SKILL_LOG_BUFF_PARTY", {
                    {"caster", caster.getName()}, {"skill", getName()}}));
                if (m_spec.isSpell) caster.consumeSpellSlot();
                return true;
            }
        }

        // === 2. 공격형 단일 / 광역 스킬 및 주문 ===
        std::vector<std::shared_ptr<Monster>> targets;
        if (m_spec.target == SkillTargetType::ALL_FOES) {
            for (auto& f : foes) {
                if (f && !f->isDead()) targets.push_back(f);
            }
        } else {
            if (targetIdx < 0 || targetIdx >= static_cast<int>(foes.size()) ||
                !foes[static_cast<std::size_t>(targetIdx)] ||
                foes[static_cast<std::size_t>(targetIdx)]->isDead()) {
                return false;
            }
            targets.push_back(foes[static_cast<std::size_t>(targetIdx)]);
        }
        if (targets.empty()) return false;

        if (m_spec.isSpell) caster.consumeSpellSlot();

        const auto weapon = caster.getEquippedItem(EquipSlot::WEAPON);
        const DamageType dmgType = (m_spec.usesWeapon && weapon) ? weapon->getDamageType() : m_spec.damageType;
        const WeaponReach reach = (m_spec.usesWeapon && weapon) ? weapon->getReach() : WeaponReach::RANGED;

        for (auto& foe : targets) {
            bool hit = m_spec.guaranteedHit;
            bool crit = false;
            if (!hit) {
                const int nat = rng.rollDie(20);
                const auto res = CombatRules::resolveAttack(caster, nat, foe->getAc(), m_spec.attackBonus);
                hit = (res.outcome != HitOutcome::MISS);
                crit = (res.outcome == HitOutcome::CRITICAL);
            }
            if (!hit) {
                logOutput.push_back(lm.format("SKILL_LOG_MISS", {
                    {"caster", caster.getName()}, {"skill", getName()}, {"target", foe->getName()}}));
                continue;
            }

            int rawDmg = 0;
            if (m_spec.usesWeapon) {
                const int diceCnt = weapon ? weapon->getDamageDiceCount() : 1;
                const int diceSides = weapon ? weapon->getDamageDiceSides() : 4;
                const int upgBonus = weapon ? weapon->getUpgradeDamageBonus() : 0;
                const int abMod = CombatRules::attackAbilityModifier(caster);
                rawDmg = CombatRules::rollAttackDamage(
                    diceCnt, diceSides, abMod, m_spec.flatBonus + upgBonus,
                    m_spec.diceCount, m_spec.diceSides, crit, rng);
                rawDmg = std::max(1, static_cast<int>(std::round(
                    static_cast<float>(rawDmg) * CombatRules::rowAttackMultiplier(caster.getRow(), reach))));
            } else {
                const int spBonus = CombatRules::spellDamageBonus(caster);
                rawDmg = CombatRules::rollAttackDamage(
                    m_spec.diceCount, m_spec.diceSides, spBonus, m_spec.flatBonus, 0, 0, crit, rng);
            }

            if (m_spec.bonusVsBrokenOrLowHp && (foe->isBroken() || foe->getHp() <= foe->getMaxHp() / 2)) {
                rawDmg = static_cast<int>(std::round(static_cast<float>(rawDmg) * 1.6f));
            }

            const auto tac = CombatRules::applyTacticalDamage(
                caster, *foe, dmgType, rawDmg, m_spec.baseStagger, true);

            if (m_spec.extraLifesteal && !caster.isDead()) {
                caster.heal(std::max(2, tac.finalDamage * 35 / 100));
            }

            std::string suffix;
            if (crit) suffix += " [CRIT]";
            if (tac.hitWeakness) suffix += " [WEAK]";
            if (tac.hitResistance) suffix += " [RESIST]";
            if (tac.triggeredBreak) suffix += " [BREAK!]";

            logOutput.push_back(lm.format("SKILL_LOG_HIT", {
                {"caster", caster.getName()}, {"skill", getName()}, {"target", foe->getName()},
                {"damage", std::to_string(tac.finalDamage) + suffix}}));

            if (!foe->isDead()) {
                if (m_spec.inflictsPoison) foe->setPoison(3);
                if (m_spec.inflictsBurn) foe->setBurn(3);
                if (m_spec.inflictsBleed) foe->setBleed(3);
                if (m_spec.inflictsFrost) foe->setFrost(3);
                if (m_spec.inflictsParalysis && rng.rollRange(1, 100) <= 65) foe->setParalysis(1);
                foe->checkPhaseTransition(logOutput);
            } else {
                logOutput.push_back(lm.format("COMBAT_LOG_DEFEATED", {{"target", foe->getName()}}));
            }
        }

        // 복합 아군 부가 효과 (예: 성기사 Radiant Aegis / 시공간술사 Time Fracture)
        if (m_spec.appliesWard || m_spec.appliesBless) {
            for (auto& a : allies) {
                if (!a || a->isDead()) continue;
                if (m_spec.appliesWard) a->applyWard(2);
                if (m_spec.appliesBless) a->applyBless(2);
            }
        }

        return true;
    }

private:
    SkillSpec m_spec;
};

} // namespace crawl

#endif // CONCRETE_SKILLS_HPP
