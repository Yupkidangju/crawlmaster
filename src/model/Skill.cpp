// [v0.8.0] Skill.cpp 신규 작성
// 12종 스킬 및 주문의 구체 실행 로직 구현. D&D 5e 스타일의 명중 판정 및 피해 롤 적용.

#include "model/ConcreteSkills.hpp"
#include "core/LocalizationManager.hpp"
#include "model/Character.hpp"
#include "model/CombatRules.hpp"
#include "model/Monster.hpp"
#include "core/SessionRng.hpp"
#include <algorithm>
#include <iostream>

namespace crawl {

std::string Skill::getName() const {
    return LocalizationManager::getInstance().getContent("SKILL", getId(), "NAME");
}

std::string Skill::getDescription() const {
    return LocalizationManager::getInstance().getContent("SKILL", getId(), "DESC");
}

// 공통 유틸리티: 주사위 굴림 함수
static int rollDice(int count, int sides) {
    if (count <= 0 || sides <= 0) return 0;
    int sum = 0;
    for (int i = 0; i < count; ++i) {
        sum += SessionRng::global().rollDie(sides);
    }
    return sum;
}

// 공통 유틸리티: d20 굴림
static int rollD20() {
    return rollDice(1, 20);
}

static void addLocalizedLog(
    std::vector<std::string>& output, const std::string& key,
    std::initializer_list<std::pair<std::string, std::string>> values = {}) {
    output.push_back(LocalizationManager::getInstance().format(key, values));
}

struct WeaponDamageSpec {
    int diceCount;
    int diceSides;
    DamageType damageType;
};

static WeaponDamageSpec weaponDamageSpec(const Character& caster, int fallbackSides,
                                         DamageType fallbackType) {
    if (const auto weapon = caster.getEquippedItem(EquipSlot::WEAPON)) {
        return {weapon->getDamageDiceCount(), weapon->getDamageDiceSides(), weapon->getDamageType()};
    }
    return {1, fallbackSides, fallbackType};
}

// ==================== 1. 전사 스킬 구현 ====================

bool SlashSkill::execute(Character& caster,
                         std::vector<std::shared_ptr<Character>>& /*allies*/,
                         std::vector<std::shared_ptr<Monster>>& foes,
                         int targetIdx,
                         std::vector<std::string>& logOutput) {
    if (targetIdx < 0 || targetIdx >= static_cast<int>(foes.size())) return false;
    auto target = foes[targetIdx];
    if (target->isDead()) return false;

    std::string casterName = caster.getName();
    std::string targetName = target->getName();

    // 명중 판정: d20 + STR 보정치 + 숙련 보너스(+2) + 스킬 보너스(+4)
    int strMod = caster.getAbilities().getModifier(caster.getAbilities().strength);
    int d20Roll = rollD20();
    int targetAc = target->getAc();
    const auto attack = CombatRules::resolveAttack(caster, d20Roll, targetAc, 4);
    int attackRoll = attack.attackTotal;

    addLocalizedLog(logOutput, "SKILL_LOG_ATTACK", {{"actor", casterName}, {"skill", getName()},
        {"roll", std::to_string(attackRoll)}, {"ac", std::to_string(targetAc)}});

    if (attack.outcome == HitOutcome::MISS && d20Roll == 1) {
        addLocalizedLog(logOutput, "COMBAT_LOG_FUMBLE");
    } else if (attack.outcome != HitOutcome::MISS) {
        const auto damageSpec = weaponDamageSpec(caster, 8, DamageType::SLASHING);
        int damage = CombatRules::rollAttackDamage(
            damageSpec.diceCount, damageSpec.diceSides, strMod, 2, 0, 0,
            attack.outcome == HitOutcome::CRITICAL, SessionRng::global());
        damage = CombatRules::mitigateDamage(*target, damageSpec.damageType, damage);
        if (attack.outcome == HitOutcome::CRITICAL) {
            addLocalizedLog(logOutput, "COMBAT_LOG_CRITICAL");
        }
        target->takeDamage(damage);
        addLocalizedLog(logOutput, "COMBAT_LOG_DAMAGE", {{"target", targetName}, {"damage", std::to_string(damage)}});
        if (target->isDead()) {
            addLocalizedLog(logOutput, "COMBAT_LOG_DEFEATED", {{"target", targetName}});
        }
    } else {
        addLocalizedLog(logOutput, "COMBAT_LOG_MISS");
    }
    return true;
}

bool ShieldBashSkill::execute(Character& caster,
                               std::vector<std::shared_ptr<Character>>& /*allies*/,
                               std::vector<std::shared_ptr<Monster>>& foes,
                               int targetIdx,
                               std::vector<std::string>& logOutput) {
    std::string casterName = caster.getName();
    
    // 방패 착용 여부 검사
    if (!caster.getEquippedItem(EquipSlot::SHIELD)) {
        addLocalizedLog(logOutput, "SKILL_LOG_SHIELD_REQUIRED");
        return false;
    }

    if (targetIdx < 0 || targetIdx >= static_cast<int>(foes.size())) return false;
    auto target = foes[targetIdx];
    if (target->isDead()) return false;

    std::string targetName = target->getName();

    // 명중 판정: d20 + STR 보정치 + 숙련 보너스(+2)
    int strMod = caster.getAbilities().getModifier(caster.getAbilities().strength);
    int d20Roll = rollD20();
    int targetAc = target->getAc();
    const auto attack = CombatRules::resolveAttack(caster, d20Roll, targetAc);
    int attackRoll = attack.attackTotal;

    addLocalizedLog(logOutput, "SKILL_LOG_ATTACK", {{"actor", casterName}, {"skill", getName()},
        {"roll", std::to_string(attackRoll)}, {"ac", std::to_string(targetAc)}});

    if (attack.outcome == HitOutcome::MISS && d20Roll == 1) {
        addLocalizedLog(logOutput, "COMBAT_LOG_FUMBLE");
    } else if (attack.outcome != HitOutcome::MISS) {
        // 피해: 1d4 + STR
        int damage = CombatRules::rollAttackDamage(
            1, 4, strMod, 0, 0, 0, attack.outcome == HitOutcome::CRITICAL,
            SessionRng::global());
        damage = CombatRules::mitigateDamage(*target, DamageType::BLUDGEONING, damage);
        target->takeDamage(damage);
        addLocalizedLog(logOutput, "COMBAT_LOG_DAMAGE", {{"target", targetName}, {"damage", std::to_string(damage)}});

        // 50% 확률로 1턴 마비 유발
        if (rollDice(1, 2) == 1) {
            target->setParalysis(1);
            addLocalizedLog(logOutput, "COMBAT_LOG_PARALYZED", {{"target", targetName}, {"turns", "1"}});
        }

        if (target->isDead()) {
            addLocalizedLog(logOutput, "COMBAT_LOG_DEFEATED", {{"target", targetName}});
        }
    } else {
        addLocalizedLog(logOutput, "COMBAT_LOG_MISS");
    }
    return true;
}

bool CleaveSkill::execute(Character& caster,
                          std::vector<std::shared_ptr<Character>>& /*allies*/,
                          std::vector<std::shared_ptr<Monster>>& foes,
                          int /*targetIdx*/,
                          std::vector<std::string>& logOutput) {
    std::string casterName = caster.getName();
    int strMod = caster.getAbilities().getModifier(caster.getAbilities().strength);
    const auto damageSpec = weaponDamageSpec(caster, 6, DamageType::SLASHING);

    addLocalizedLog(logOutput, "SKILL_LOG_CAST", {{"actor", casterName}, {"skill", getName()}});

    bool hitAny = false;
    bool attempted = false;
    for (auto& target : foes) {
        if (target && !target->isDead()) {
            attempted = true;
            std::string targetName = target->getName();
            int d20Roll = rollD20();
            int targetAc = target->getAc();
            const auto attack = CombatRules::resolveAttack(caster, d20Roll, targetAc);

            if (attack.outcome != HitOutcome::MISS) {
                int damage = CombatRules::rollAttackDamage(
                    damageSpec.diceCount, damageSpec.diceSides, strMod, 0, 0, 0,
                    attack.outcome == HitOutcome::CRITICAL, SessionRng::global());
                damage = CombatRules::mitigateDamage(*target, damageSpec.damageType, damage);
                target->takeDamage(damage);
                addLocalizedLog(logOutput, "COMBAT_LOG_DAMAGE", {{"target", targetName}, {"damage", std::to_string(damage)}});
                hitAny = true;
                if (target->isDead()) {
                    addLocalizedLog(logOutput, "COMBAT_LOG_DEFEATED", {{"target", targetName}});
                }
            } else {
                addLocalizedLog(logOutput, "COMBAT_LOG_TARGET_MISS", {{"target", targetName}});
            }
        }
    }

    if (!hitAny) {
        addLocalizedLog(logOutput, "COMBAT_LOG_NO_TARGET");
    }
    return attempted;
}


// ==================== 2. 마법사 주문 구현 ====================

bool MagicMissileSpell::execute(Character& caster,
                                std::vector<std::shared_ptr<Character>>& /*allies*/,
                                std::vector<std::shared_ptr<Monster>>& foes,
                                int targetIdx,
                                std::vector<std::string>& logOutput) {
    if (caster.getSpellSlots() <= 0) {
        addLocalizedLog(logOutput, "SKILL_LOG_NO_SLOTS");
        return false;
    }

    if (targetIdx < 0 || targetIdx >= static_cast<int>(foes.size())) return false;
    auto target = foes[targetIdx];
    if (target->isDead()) return false;

    caster.consumeSpellSlot();
    std::string casterName = caster.getName();
    std::string targetName = target->getName();

    // 100% 필중: 1d4 + INT + 1 마법 대미지
    int intMod = caster.getAbilities().getModifier(caster.getAbilities().intelligence);
    int damage = rollDice(1, 4) + intMod + 1;
    damage = std::max(1, damage);

    target->takeDamage(damage);
    addLocalizedLog(logOutput, "SKILL_LOG_CAST", {{"actor", casterName}, {"skill", getName()}});
    addLocalizedLog(logOutput, "COMBAT_LOG_DAMAGE", {{"target", targetName}, {"damage", std::to_string(damage)}});

    if (target->isDead()) {
        addLocalizedLog(logOutput, "COMBAT_LOG_DEFEATED", {{"target", targetName}});
    }
    return true;
}

bool SleepSpell::execute(Character& caster,
                         std::vector<std::shared_ptr<Character>>& /*allies*/,
                         std::vector<std::shared_ptr<Monster>>& foes,
                         int targetIdx,
                         std::vector<std::string>& logOutput) {
    if (caster.getSpellSlots() <= 0) {
        addLocalizedLog(logOutput, "SKILL_LOG_NO_SLOTS");
        return false;
    }

    if (targetIdx < 0 || targetIdx >= static_cast<int>(foes.size())) return false;
    auto target = foes[targetIdx];
    if (target->isDead()) return false;

    caster.consumeSpellSlot();
    std::string casterName = caster.getName();
    std::string targetName = target->getName();

    // 2턴간 마비(수면) 유발
    target->setParalysis(2);
    addLocalizedLog(logOutput, "SKILL_LOG_CAST", {{"actor", casterName}, {"skill", getName()}});
    addLocalizedLog(logOutput, "COMBAT_LOG_PARALYZED", {{"target", targetName}, {"turns", "2"}});
    return true;
}

bool FireballSpell::execute(Character& caster,
                            std::vector<std::shared_ptr<Character>>& /*allies*/,
                            std::vector<std::shared_ptr<Monster>>& foes,
                            int /*targetIdx*/,
                            std::vector<std::string>& logOutput) {
    if (caster.getSpellSlots() <= 0) {
        addLocalizedLog(logOutput, "SKILL_LOG_NO_SLOTS");
        return false;
    }

    const bool hasTarget = std::any_of(foes.begin(), foes.end(),
        [](const auto& target) { return target && !target->isDead(); });
    if (!hasTarget) return false;

    caster.consumeSpellSlot();
    std::string casterName = caster.getName();
    int intMod = caster.getAbilities().getModifier(caster.getAbilities().intelligence);

    addLocalizedLog(logOutput, "SKILL_LOG_CAST", {{"actor", casterName}, {"skill", getName()}});

    for (auto& target : foes) {
        if (target && !target->isDead()) {
            std::string targetName = target->getName();
            // 각 적에게 2d6 + 지능 대미지
            int damage = rollDice(2, 6) + intMod;
            damage = std::max(1, damage);
            target->takeDamage(damage);
            addLocalizedLog(logOutput, "COMBAT_LOG_DAMAGE", {{"target", targetName}, {"damage", std::to_string(damage)}});
            if (target->isDead()) {
                addLocalizedLog(logOutput, "COMBAT_LOG_DEFEATED", {{"target", targetName}});
            }
        }
    }
    return true;
}


// ==================== 3. 도적 스킬 구현 ====================

bool SneakAttackSkill::execute(Character& caster,
                               std::vector<std::shared_ptr<Character>>& /*allies*/,
                               std::vector<std::shared_ptr<Monster>>& foes,
                               int targetIdx,
                               std::vector<std::string>& logOutput) {
    if (targetIdx < 0 || targetIdx >= static_cast<int>(foes.size())) return false;
    auto target = foes[targetIdx];
    if (target->isDead()) return false;

    std::string casterName = caster.getName();
    std::string targetName = target->getName();

    // 명중 판정: d20 + DEX 보정치 + 숙련보너스(+2) + 기습보너스(+2)
    int dexMod = caster.getAbilities().getModifier(caster.getAbilities().dexterity);
    int d20Roll = rollD20();
    int targetAc = target->getAc();
    const auto attack = CombatRules::resolveAttack(caster, d20Roll, targetAc, 2);
    int attackRoll = attack.attackTotal;

    addLocalizedLog(logOutput, "SKILL_LOG_ATTACK", {{"actor", casterName}, {"skill", getName()},
        {"roll", std::to_string(attackRoll)}, {"ac", std::to_string(targetAc)}});

    if (attack.outcome == HitOutcome::MISS && d20Roll == 1) {
        addLocalizedLog(logOutput, "COMBAT_LOG_FUMBLE");
    } else if (attack.outcome != HitOutcome::MISS) {
        const auto damageSpec = weaponDamageSpec(caster, 4, DamageType::PIERCING);
        int damage = CombatRules::rollAttackDamage(
            damageSpec.diceCount, damageSpec.diceSides, dexMod, 0, 2, 4,
            attack.outcome == HitOutcome::CRITICAL, SessionRng::global());
        damage = CombatRules::mitigateDamage(*target, damageSpec.damageType, damage);

        if (attack.outcome == HitOutcome::CRITICAL) {
            addLocalizedLog(logOutput, "COMBAT_LOG_CRITICAL");
        }

        target->takeDamage(damage);
        addLocalizedLog(logOutput, "COMBAT_LOG_DAMAGE", {{"target", targetName}, {"damage", std::to_string(damage)}});

        if (target->isDead()) {
            addLocalizedLog(logOutput, "COMBAT_LOG_DEFEATED", {{"target", targetName}});
        }
    } else {
        addLocalizedLog(logOutput, "COMBAT_LOG_MISS");
    }
    return true;
}

bool PoisonDartSkill::execute(Character& caster,
                              std::vector<std::shared_ptr<Character>>& /*allies*/,
                              std::vector<std::shared_ptr<Monster>>& foes,
                              int targetIdx,
                              std::vector<std::string>& logOutput) {
    if (targetIdx < 0 || targetIdx >= static_cast<int>(foes.size())) return false;
    auto target = foes[targetIdx];
    if (target->isDead()) return false;

    std::string casterName = caster.getName();
    std::string targetName = target->getName();

    int dexMod = caster.getAbilities().getModifier(caster.getAbilities().dexterity);
    int d20Roll = rollD20();
    int targetAc = target->getAc();
    const auto attack = CombatRules::resolveAttack(caster, d20Roll, targetAc);
    int attackRoll = attack.attackTotal;

    addLocalizedLog(logOutput, "SKILL_LOG_ATTACK", {{"actor", casterName}, {"skill", getName()},
        {"roll", std::to_string(attackRoll)}, {"ac", std::to_string(targetAc)}});

    if (attack.outcome != HitOutcome::MISS) {
        int damage = CombatRules::rollAttackDamage(
            1, 4, dexMod, 0, 0, 0, attack.outcome == HitOutcome::CRITICAL,
            SessionRng::global());
        damage = CombatRules::mitigateDamage(*target, DamageType::PIERCING, damage);
        target->takeDamage(damage);
        addLocalizedLog(logOutput, "COMBAT_LOG_DAMAGE", {{"target", targetName}, {"damage", std::to_string(damage)}});
        
        // 3턴간 독 부여
        target->setPoison(3);
        addLocalizedLog(logOutput, "COMBAT_LOG_POISONED", {{"target", targetName}, {"turns", "3"}});

        if (target->isDead()) {
            addLocalizedLog(logOutput, "COMBAT_LOG_DEFEATED", {{"target", targetName}});
        }
    } else {
        addLocalizedLog(logOutput, "COMBAT_LOG_MISS");
    }
    return true;
}

bool ShadowstrikeSkill::execute(Character& caster,
                                std::vector<std::shared_ptr<Character>>& /*allies*/,
                                std::vector<std::shared_ptr<Monster>>& foes,
                                int targetIdx,
                                std::vector<std::string>& logOutput) {
    if (targetIdx < 0 || targetIdx >= static_cast<int>(foes.size())) return false;
    auto target = foes[targetIdx];
    if (target->isDead()) return false;

    std::string casterName = caster.getName();
    std::string targetName = target->getName();

    int dexMod = caster.getAbilities().getModifier(caster.getAbilities().dexterity);
    int d20Roll = rollD20();
    int targetAc = target->getAc();
    const auto attack = CombatRules::resolveAttack(caster, d20Roll, targetAc, 1);
    int attackRoll = attack.attackTotal;

    addLocalizedLog(logOutput, "SKILL_LOG_ATTACK", {{"actor", casterName}, {"skill", getName()},
        {"roll", std::to_string(attackRoll)}, {"ac", std::to_string(targetAc)}});

    if (attack.outcome != HitOutcome::MISS) {
        const auto damageSpec = weaponDamageSpec(caster, 4, DamageType::PIERCING);
        int damage = CombatRules::rollAttackDamage(
            damageSpec.diceCount, damageSpec.diceSides, dexMod, 0, 3, 4,
            attack.outcome == HitOutcome::CRITICAL, SessionRng::global());
        damage = CombatRules::mitigateDamage(*target, damageSpec.damageType, damage);

        if (attack.outcome == HitOutcome::CRITICAL) {
            addLocalizedLog(logOutput, "COMBAT_LOG_CRITICAL");
        }

        target->takeDamage(damage);
        addLocalizedLog(logOutput, "COMBAT_LOG_DAMAGE", {{"target", targetName}, {"damage", std::to_string(damage)}});

        if (target->isDead()) {
            addLocalizedLog(logOutput, "COMBAT_LOG_DEFEATED", {{"target", targetName}});
        }
    } else {
        addLocalizedLog(logOutput, "COMBAT_LOG_MISS");
    }
    return true;
}


// ==================== 4. 성직자 주문 구현 ====================

bool CureWoundsSpell::execute(Character& caster,
                              std::vector<std::shared_ptr<Character>>& allies,
                              std::vector<std::shared_ptr<Monster>>& /*foes*/,
                              int targetIdx,
                              std::vector<std::string>& logOutput) {
    if (caster.getSpellSlots() <= 0) {
        addLocalizedLog(logOutput, "SKILL_LOG_NO_SLOTS");
        return false;
    }

    if (targetIdx < 0 || targetIdx >= static_cast<int>(allies.size())) return false;
    auto healTarget = allies[static_cast<std::size_t>(targetIdx)];
    if (!healTarget || healTarget->isDead() || healTarget->getHp() >= healTarget->getMaxHp()) return false;

    caster.consumeSpellSlot();
    std::string casterName = caster.getName();
    std::string targetName = healTarget->getName();

    int wisMod = caster.getAbilities().getModifier(caster.getAbilities().wisdom);
    int healAmount = rollDice(1, 8) + std::max(0, wisMod);

    healTarget->heal(healAmount);
    addLocalizedLog(logOutput, "SKILL_LOG_CAST", {{"actor", casterName}, {"skill", getName()}});
    addLocalizedLog(logOutput, "COMBAT_LOG_HEALED", {{"target", targetName}, {"amount", std::to_string(healAmount)}});
    return true;
}

bool BlessSpell::execute(Character& caster,
                         std::vector<std::shared_ptr<Character>>& allies,
                         std::vector<std::shared_ptr<Monster>>& /*foes*/,
                         int /*targetIdx*/,
                         std::vector<std::string>& logOutput) {
    if (caster.getSpellSlots() <= 0) {
        addLocalizedLog(logOutput, "SKILL_LOG_NO_SLOTS");
        return false;
    }

    const bool hasTarget = std::any_of(allies.begin(), allies.end(),
        [](const auto& member) { return member && !member->isDead(); });
    if (!hasTarget) return false;

    caster.consumeSpellSlot();
    std::string casterName = caster.getName();
    addLocalizedLog(logOutput, "SKILL_LOG_CAST", {{"actor", casterName}, {"skill", getName()}});

    // 아군 전체에게 명중 +2 버프 부여
    for (auto& member : allies) {
        if (member && !member->isDead()) {
            member->applyBless(3); // 3턴간 지속
            addLocalizedLog(logOutput, "COMBAT_LOG_BLESSED", {{"target", member->getName()}});
        }
    }
    return true;
}

bool PrayerOfHealingSpell::execute(Character& caster,
                                    std::vector<std::shared_ptr<Character>>& allies,
                                    std::vector<std::shared_ptr<Monster>>& /*foes*/,
                                    int /*targetIdx*/,
                                    std::vector<std::string>& logOutput) {
    if (caster.getSpellSlots() <= 0) {
        addLocalizedLog(logOutput, "SKILL_LOG_NO_SLOTS");
        return false;
    }

    const bool hasInjuredTarget = std::any_of(allies.begin(), allies.end(),
        [](const auto& member) {
            return member && !member->isDead() && member->getHp() < member->getMaxHp();
        });
    if (!hasInjuredTarget) return false;

    caster.consumeSpellSlot();
    std::string casterName = caster.getName();
    int wisMod = caster.getAbilities().getModifier(caster.getAbilities().wisdom);

    addLocalizedLog(logOutput, "SKILL_LOG_CAST", {{"actor", casterName}, {"skill", getName()}});

    for (auto& member : allies) {
        if (member && !member->isDead() && member->getHp() < member->getMaxHp()) {
            int healAmount = rollDice(2, 6) + std::max(0, wisMod);
            member->heal(healAmount);
            addLocalizedLog(logOutput, "COMBAT_LOG_HEALED", {{"target", member->getName()}, {"amount", std::to_string(healAmount)}});
        }
    }
    return true;
}

} // namespace crawl
