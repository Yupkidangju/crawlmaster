// [v0.8.0] ConcreteSkills.hpp 신규 작성
// 전사, 마법사, 도적, 성직자의 12종 액티브 스킬 및 주문 구체 클래스 정의.

#ifndef CONCRETE_SKILLS_HPP
#define CONCRETE_SKILLS_HPP

#include "model/Skill.hpp"

namespace crawl {

// ==================== 1. 전사 스킬 (Warrior Skills) ====================

// Slash: 1d8 + STR + 2 물리 피해 및 명중 +4 가산 보정 스킬 (Lv.1)
class SlashSkill : public Skill {
public:
    std::string getId() const override { return "skl_slash"; }
    int getRequiredLevel() const override { return 1; }
    bool isSpell() const override { return false; }
    int getSpellLevel() const override { return 0; }
    SkillTargetType getTargetType() const override { return SkillTargetType::SINGLE_FOE; }

    bool execute(Character& caster,
                 std::vector<std::shared_ptr<Character>>& allies,
                 std::vector<std::shared_ptr<Monster>>& foes,
                 int targetIdx,
                 std::vector<std::string>& logOutput) override;
};

// Shield Bash: 방패 타격 1d4 + STR 피해 및 50% 확률로 1턴 마비 기절 유발 (Lv.2)
class ShieldBashSkill : public Skill {
public:
    std::string getId() const override { return "skl_shield_bash"; }
    int getRequiredLevel() const override { return 2; }
    bool isSpell() const override { return false; }
    int getSpellLevel() const override { return 0; }
    SkillTargetType getTargetType() const override { return SkillTargetType::SINGLE_FOE; }

    bool execute(Character& caster,
                 std::vector<std::shared_ptr<Character>>& allies,
                 std::vector<std::shared_ptr<Monster>>& foes,
                 int targetIdx,
                 std::vector<std::string>& logOutput) override;
};

// Cleave: 적 전체 휩쓸기 1d6 + STR 대미지 (Lv.3)
class CleaveSkill : public Skill {
public:
    std::string getId() const override { return "skl_cleave"; }
    int getRequiredLevel() const override { return 3; }
    bool isSpell() const override { return false; }
    int getSpellLevel() const override { return 0; }
    SkillTargetType getTargetType() const override { return SkillTargetType::ALL_FOES; }

    bool execute(Character& caster,
                 std::vector<std::shared_ptr<Character>>& allies,
                 std::vector<std::shared_ptr<Monster>>& foes,
                 int targetIdx,
                 std::vector<std::string>& logOutput) override;
};


// ==================== 2. 마법사 주문 (Mage Spells) ====================

// Magic Missile: 100% 명중하는 1d4 + INT + 1 마법 피해 주문 (Lv.1)
class MagicMissileSpell : public Skill {
public:
    std::string getId() const override { return "spl_magic_missile"; }
    int getRequiredLevel() const override { return 1; }
    bool isSpell() const override { return true; }
    int getSpellLevel() const override { return 1; }
    SkillTargetType getTargetType() const override { return SkillTargetType::SINGLE_FOE; }

    bool execute(Character& caster,
                 std::vector<std::shared_ptr<Character>>& allies,
                 std::vector<std::shared_ptr<Monster>>& foes,
                 int targetIdx,
                 std::vector<std::string>& logOutput) override;
};

// Sleep: 적 한 명에게 2턴간 마비(수면) 효과를 유발하는 주문 (Lv.2)
class SleepSpell : public Skill {
public:
    std::string getId() const override { return "spl_sleep"; }
    int getRequiredLevel() const override { return 2; }
    bool isSpell() const override { return true; }
    int getSpellLevel() const override { return 1; }
    SkillTargetType getTargetType() const override { return SkillTargetType::SINGLE_FOE; }

    bool execute(Character& caster,
                 std::vector<std::shared_ptr<Character>>& allies,
                 std::vector<std::shared_ptr<Monster>>& foes,
                 int targetIdx,
                 std::vector<std::string>& logOutput) override;
};

// Fireball: 적 전체에게 폭발적 파이어볼을 투사하여 2d6 + INT 화염 피해 주문 (Lv.3)
class FireballSpell : public Skill {
public:
    std::string getId() const override { return "spl_fireball"; }
    int getRequiredLevel() const override { return 3; }
    bool isSpell() const override { return true; }
    int getSpellLevel() const override { return 2; }
    SkillTargetType getTargetType() const override { return SkillTargetType::ALL_FOES; }

    bool execute(Character& caster,
                 std::vector<std::shared_ptr<Character>>& allies,
                 std::vector<std::shared_ptr<Monster>>& foes,
                 int targetIdx,
                 std::vector<std::string>& logOutput) override;
};


// ==================== 3. 도적 스킬 (Rogue Skills) ====================

// Sneak Attack: DEX 보정 기습 물리 피해 1d4 + 2d4 + DEX (Lv.1)
class SneakAttackSkill : public Skill {
public:
    std::string getId() const override { return "skl_sneak_attack"; }
    int getRequiredLevel() const override { return 1; }
    bool isSpell() const override { return false; }
    int getSpellLevel() const override { return 0; }
    SkillTargetType getTargetType() const override { return SkillTargetType::SINGLE_FOE; }

    bool execute(Character& caster,
                 std::vector<std::shared_ptr<Character>>& allies,
                 std::vector<std::shared_ptr<Monster>>& foes,
                 int targetIdx,
                 std::vector<std::string>& logOutput) override;
};

// Poison Dart: 적 단일 대상 1d4 관통 피해 및 3턴간 매턴 독 피해(1d3) 디버프 유발 (Lv.2)
class PoisonDartSkill : public Skill {
public:
    std::string getId() const override { return "skl_poison_dart"; }
    int getRequiredLevel() const override { return 2; }
    bool isSpell() const override { return false; }
    int getSpellLevel() const override { return 0; }
    SkillTargetType getTargetType() const override { return SkillTargetType::SINGLE_FOE; }

    bool execute(Character& caster,
                 std::vector<std::shared_ptr<Character>>& allies,
                 std::vector<std::shared_ptr<Monster>>& foes,
                 int targetIdx,
                 std::vector<std::string>& logOutput) override;
};

// Shadowstrike: 적 1명에게 강력한 DEX 기반 은신 일격 1d8 + 3d4 + DEX 피해 (Lv.3)
class ShadowstrikeSkill : public Skill {
public:
    std::string getId() const override { return "skl_shadowstrike"; }
    int getRequiredLevel() const override { return 3; }
    bool isSpell() const override { return false; }
    int getSpellLevel() const override { return 0; }
    SkillTargetType getTargetType() const override { return SkillTargetType::SINGLE_FOE; }

    bool execute(Character& caster,
                 std::vector<std::shared_ptr<Character>>& allies,
                 std::vector<std::shared_ptr<Monster>>& foes,
                 int targetIdx,
                 std::vector<std::string>& logOutput) override;
};


// ==================== 4. 성직자 주문 (Cleric Spells) ====================

// Cure Wounds: 가장 체력 비율이 낮은 아군 1명을 1d8 + WIS 수치만큼 치유 주문 (Lv.1)
class CureWoundsSpell : public Skill {
public:
    std::string getId() const override { return "spl_cure_wounds"; }
    int getRequiredLevel() const override { return 1; }
    bool isSpell() const override { return true; }
    int getSpellLevel() const override { return 1; }
    SkillTargetType getTargetType() const override { return SkillTargetType::SINGLE_ALLY; }

    bool execute(Character& caster,
                 std::vector<std::shared_ptr<Character>>& allies,
                 std::vector<std::shared_ptr<Monster>>& foes,
                 int targetIdx,
                 std::vector<std::string>& logOutput) override;
};

// Bless: 아군 전체에게 3턴간 축복 버프(공격 명중 롤 +2 가산) 주문 (Lv.2)
class BlessSpell : public Skill {
public:
    std::string getId() const override { return "spl_bless"; }
    int getRequiredLevel() const override { return 2; }
    bool isSpell() const override { return true; }
    int getSpellLevel() const override { return 1; }
    SkillTargetType getTargetType() const override { return SkillTargetType::ALL_ALLIES; }

    bool execute(Character& caster,
                 std::vector<std::shared_ptr<Character>>& allies,
                 std::vector<std::shared_ptr<Monster>>& foes,
                 int targetIdx,
                 std::vector<std::string>& logOutput) override;
};

// Prayer of Healing: 아군 전체 2d6 + WIS HP 일괄 치유 주문 (Lv.3)
class PrayerOfHealingSpell : public Skill {
public:
    std::string getId() const override { return "spl_prayer_of_healing"; }
    int getRequiredLevel() const override { return 3; }
    bool isSpell() const override { return true; }
    int getSpellLevel() const override { return 2; }
    SkillTargetType getTargetType() const override { return SkillTargetType::ALL_ALLIES; }

    bool execute(Character& caster,
                 std::vector<std::shared_ptr<Character>>& allies,
                 std::vector<std::shared_ptr<Monster>>& foes,
                 int targetIdx,
                 std::vector<std::string>& logOutput) override;
};

} // namespace crawl

#endif // CONCRETE_SKILLS_HPP
