#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include <array>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "model/DungeonMap.hpp"
#include "model/Equipment.hpp"

namespace crawl {

class Skill;

enum class CharacterClass {
    WARRIOR = 0,
    MAGE = 1,
    ROGUE = 2,
    CLERIC = 3
};

enum class Gender {
    MALE = 0,
    FEMALE = 1,
    NON_BINARY = 2,
    UNSPECIFIED = 3
};

enum class Ability {
    STRENGTH = 0,
    DEXTERITY = 1,
    CONSTITUTION = 2,
    INTELLIGENCE = 3,
    WISDOM = 4,
    CHARISMA = 5
};

enum class CombatRow {
    FRONT = 0,
    BACK = 1
};

enum class Specialization {
    NONE = 0,
    VANGUARD = 1,      // Warrior: 방벽의 기사
    BERSERKER = 2,     // Warrior: 광전사
    PYROMANCER = 3,    // Mage: 화염술사
    CHRONOMANCER = 4,  // Mage: 시공간술사
    ASSASSIN = 5,      // Rogue: 암살자
    TRAPSMITH = 6,     // Rogue: 기공사
    PALADIN = 7,       // Cleric: 성기사
    ORACLE = 8         // Cleric: 신탁사
};

enum class TalentNode {
    VITALITY = 0, // 최대 HP 및 내구력 강화
    MASTERY = 1,  // 무기/주문 공격력 강화
    TACTICS = 2,  // AC·우선권·스태거 위력 강화
    APEX = 3      // 치명타·치유·속성 특화
};

struct CharacterIdentity {
    std::string name;
    int age = 20;
    Gender gender = Gender::MALE;
    CharacterClass characterClass = CharacterClass::WARRIOR;
};

struct AbilityScore {
    int strength = 10;
    int dexterity = 10;
    int constitution = 10;
    int intelligence = 10;
    int wisdom = 10;
    int charisma = 10;

    [[nodiscard]] int getModifier(int score) const {
        const int diff = score - 10;
        return (diff < 0) ? (diff - 1) / 2 : diff / 2;
    }
};

class Character {
public:
    static constexpr int MAX_LEVEL = 10;
    static constexpr int MAX_TALENT_RANK = 3;
    static constexpr int SPEC_UNLOCK_LEVEL = 3;

    Character(std::string name, CharacterClass charClass);
    Character(CharacterIdentity identity, AbilityScore abilities);
    ~Character() = default;

    void rollAbilities();
    [[nodiscard]] int getAc() const;
    [[nodiscard]] bool canEquip(const Equipment& equipItem) const;
    bool equip(std::shared_ptr<Equipment> equipItem);
    void unequip(EquipSlot slot);

    bool addXp(int amount);
    [[nodiscard]] int getNextLevelXp() const;
    void rest();
    void sanctuaryRest();
    void heal(int amount);
    void revive(int hpAmount);
    void takeDamage(int damage);
    [[nodiscard]] bool isDead() const;

    [[nodiscard]] std::string getName() const;
    [[nodiscard]] int getAge() const;
    [[nodiscard]] Gender getGender() const;
    [[nodiscard]] CharacterClass getClass() const;
    [[nodiscard]] std::string getClassString() const;
    [[nodiscard]] std::string getClassSlug() const;
    [[nodiscard]] int getLevel() const;
    [[nodiscard]] int getXp() const;
    [[nodiscard]] int getHp() const;
    [[nodiscard]] int getMaxHp() const;
    [[nodiscard]] int getSpellSlots() const;
    [[nodiscard]] int getMaxSpellSlots() const;
    void consumeSpellSlot();
    void recoverSpellSlot(int amount = 1);

    [[nodiscard]] AbilityScore getAbilities() const;
    [[nodiscard]] AbilityScore getRawAbilities() const;
    [[nodiscard]] std::shared_ptr<Equipment> getEquippedItem(EquipSlot slot) const;

    // 진형 (전열/후열)
    [[nodiscard]] CombatRow getRow() const { return m_row; }
    void setRow(CombatRow row) { m_row = row; }
    void toggleRow() { m_row = (m_row == CombatRow::FRONT) ? CombatRow::BACK : CombatRow::FRONT; }

    // 전직 및 특성 트리
    [[nodiscard]] Specialization getSpecialization() const { return m_specialization; }
    [[nodiscard]] bool canChooseSpecialization() const;
    [[nodiscard]] std::pair<Specialization, Specialization> getAvailableSpecializations() const;
    bool chooseSpecialization(Specialization spec);
    [[nodiscard]] std::string getSpecializationName() const;

    [[nodiscard]] int getTalentPoints() const { return m_talentPoints; }
    [[nodiscard]] int getTalentRank(TalentNode node) const;
    bool investTalent(TalentNode node);
    [[nodiscard]] int getCritThreshold() const;
    [[nodiscard]] int getLifestealPct() const;
    [[nodiscard]] int getStaggerBonus() const;

    // 상태이상 & 전투 버프/태세
    void setPoison(int turns);
    void setParalysis(int turns);
    void setBurn(int turns);
    void setBleed(int turns);
    void setFrost(int turns);
    [[nodiscard]] int getPoisonTurns() const;
    [[nodiscard]] int getParalysisTurns() const;
    [[nodiscard]] int getBurnTurns() const;
    [[nodiscard]] int getBleedTurns() const;
    [[nodiscard]] int getFrostTurns() const;
    [[nodiscard]] bool isParalyzed() const;
    [[nodiscard]] bool hasAnyDebuff() const;
    void cureAllDebuffs();

    void setGuarding(bool guarding) { m_guarding = guarding; }
    [[nodiscard]] bool isGuarding() const { return m_guarding; }
    void setTaunt(int turns) { m_tauntTurns = std::max(m_tauntTurns, turns); }
    [[nodiscard]] int getTauntTurns() const { return m_tauntTurns; }
    void applyWard(int turns) { m_wardTurns = std::max(m_wardTurns, turns); }
    [[nodiscard]] int getWardTurns() const { return m_wardTurns; }

    void applyStrBuff(int amount, int turns);
    void applyDexBuff(int amount, int turns);
    void applyBless(int turns);
    [[nodiscard]] int getStrBuffAmount() const;
    [[nodiscard]] int getDexBuffAmount() const;
    [[nodiscard]] int getStrBuffTurns() const;
    [[nodiscard]] int getDexBuffTurns() const;
    [[nodiscard]] int getBlessTurns() const;

    void processTurnEffects(std::vector<std::string>& logOutput);
    void advanceCombatBuffDurations(std::vector<std::string>& logOutput);
    void clearCombatBuffs();

    void learnSkill(std::shared_ptr<Skill> skill);
    [[nodiscard]] const std::vector<std::shared_ptr<Skill>>& getSkills() const;
    void initSkillsForLevel();

    [[nodiscard]] nlohmann::json toJson() const;
    static std::unique_ptr<Character> fromJson(const nlohmann::json& j);

private:
    void initBaseStats();
    void recalculateDerivedMaximums(bool healDelta = false);
    [[nodiscard]] int computeMaxHp() const;
    [[nodiscard]] int computeMaxSpellSlots() const;

    std::string m_name;
    int m_age = 20;
    Gender m_gender = Gender::MALE;
    CharacterClass m_class = CharacterClass::WARRIOR;
    Specialization m_specialization = Specialization::NONE;
    CombatRow m_row = CombatRow::FRONT;

    int m_level = 1;
    int m_xp = 0;
    int m_hp = 10;
    int m_maxHp = 10;
    int m_baseHpRolls = 0;
    int m_spellSlots = 0;
    int m_maxSpellSlots = 0;

    int m_talentPoints = 0;
    std::array<int, 4> m_talents{0, 0, 0, 0};

    AbilityScore m_abilities;

    std::shared_ptr<Equipment> m_weapon;
    std::shared_ptr<Equipment> m_armor;
    std::shared_ptr<Equipment> m_shield;
    std::shared_ptr<Equipment> m_accessory;

    int m_poisonTurns = 0;
    int m_paralysisTurns = 0;
    int m_burnTurns = 0;
    int m_bleedTurns = 0;
    int m_frostTurns = 0;

    bool m_guarding = false;
    int m_tauntTurns = 0;
    int m_wardTurns = 0;
    int m_strBuffAmount = 0;
    int m_strBuffTurns = 0;
    int m_dexBuffAmount = 0;
    int m_dexBuffTurns = 0;
    int m_blessTurns = 0;

    std::vector<std::shared_ptr<Skill>> m_skills;
};

} // namespace crawl

#endif // CHARACTER_HPP
