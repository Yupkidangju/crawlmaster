#include "model/Character.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/CharacterIdentityRules.hpp"
#include "model/ItemFactory.hpp"
#include "model/Skill.hpp"
#include "model/SkillFactory.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace crawl {
namespace {

int hitDieForClass(CharacterClass c) {
    switch (c) {
        case CharacterClass::WARRIOR: return 10;
        case CharacterClass::CLERIC:  return 8;
        case CharacterClass::ROGUE:   return 8;
        case CharacterClass::MAGE:    return 6;
    }
    return 8;
}

bool isTwoHandedWeaponId(const std::string& id) {
    return id == "wpn_greatsword" || id == "wpn_halberd" || id == "wpn_longbow" ||
           id == "wpn_stormbow" || id == "wpn_dragonbane" || id == "wpn_obsidian_axe";
}

bool isHeavyArmorId(const std::string& id) {
    return id == "arm_chain" || id == "arm_plate" || id == "arm_obsidian_plate" ||
           id == "arm_dragonscale";
}

} // namespace

Character::Character(std::string name, CharacterClass charClass)
    : m_name(std::move(name)), m_class(charClass) {
    m_row = (charClass == CharacterClass::MAGE || charClass == CharacterClass::ROGUE)
                ? CombatRow::BACK
                : CombatRow::FRONT;
    rollAbilities();
    initBaseStats();
}

Character::Character(CharacterIdentity identity, AbilityScore abilities)
    : m_name(std::move(identity.name)),
      m_age(identity.age),
      m_gender(identity.gender),
      m_class(identity.characterClass),
      m_abilities(abilities) {
    m_row = (m_class == CharacterClass::MAGE || m_class == CharacterClass::ROGUE)
                ? CombatRow::BACK
                : CombatRow::FRONT;
    initBaseStats();
}

void Character::rollAbilities() {
    auto roll4d6DropLowest = []() -> int {
        std::array<int, 4> rolls{};
        for (int& r : rolls) r = SessionRng::global().rollDie(6);
        std::sort(rolls.begin(), rolls.end());
        return rolls[1] + rolls[2] + rolls[3];
    };

    m_abilities.strength = roll4d6DropLowest();
    m_abilities.dexterity = roll4d6DropLowest();
    m_abilities.constitution = roll4d6DropLowest();
    m_abilities.intelligence = roll4d6DropLowest();
    m_abilities.wisdom = roll4d6DropLowest();
    m_abilities.charisma = roll4d6DropLowest();
}

void Character::initBaseStats() {
    m_baseHpRolls = hitDieForClass(m_class);
    m_maxHp = computeMaxHp();
    m_hp = m_maxHp;
    m_maxSpellSlots = computeMaxSpellSlots();
    m_spellSlots = m_maxSpellSlots;

    // 시작 장비 보급
    if (m_class == CharacterClass::WARRIOR) {
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("wpn_longsword")));
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("arm_scale")));
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("shd_round")));
    } else if (m_class == CharacterClass::MAGE) {
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("wpn_staff")));
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("arm_robe")));
    } else if (m_class == CharacterClass::ROGUE) {
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("wpn_dagger")));
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("arm_leather")));
    } else if (m_class == CharacterClass::CLERIC) {
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("wpn_mace")));
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("arm_scale")));
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("shd_round")));
    }

    initSkillsForLevel();
}

int Character::computeMaxHp() const {
    const AbilityScore eff = getAbilities();
    const int conMod = eff.getModifier(eff.constitution);
    int hp = std::max(4, m_baseHpRolls + conMod * m_level);
    hp += getTalentRank(TalentNode::VITALITY) * 6;
    if (m_specialization == Specialization::VANGUARD) hp += 12;
    if (m_specialization == Specialization::BERSERKER) hp += 8;
    if (m_specialization == Specialization::PALADIN) hp += 10;
    for (const auto& eq : {m_weapon, m_armor, m_shield, m_accessory}) {
        if (eq) hp += eq->getBonusHp();
    }
    return std::max(6, hp);
}

int Character::computeMaxSpellSlots() const {
    int base = 0;
    if (m_class == CharacterClass::MAGE || m_class == CharacterClass::CLERIC) {
        base = 2 + m_level;
        if (m_specialization == Specialization::ORACLE) base += 2;
        if (m_specialization == Specialization::CHRONOMANCER) base += 1;
        base += getTalentRank(TalentNode::APEX) / 2;
    } else if (m_specialization == Specialization::PALADIN) {
        base = 2 + m_level / 2;
    }
    return base;
}

void Character::recalculateDerivedMaximums(bool healDelta) {
    const int oldMax = m_maxHp;
    m_maxHp = computeMaxHp();
    if (healDelta && m_maxHp > oldMax && !isDead()) {
        m_hp = std::min(m_maxHp, m_hp + (m_maxHp - oldMax));
    } else {
        m_hp = std::min(m_hp, m_maxHp);
    }

    const int oldSlots = m_maxSpellSlots;
    m_maxSpellSlots = computeMaxSpellSlots();
    if (healDelta && m_maxSpellSlots > oldSlots) {
        m_spellSlots = std::min(m_maxSpellSlots, m_spellSlots + (m_maxSpellSlots - oldSlots));
    } else {
        m_spellSlots = std::min(m_spellSlots, m_maxSpellSlots);
    }
}

int Character::getAc() const {
    const AbilityScore eff = getAbilities();
    int ac = 10 + eff.getModifier(eff.dexterity);
    if (m_class == CharacterClass::WARRIOR) ac += 1;
    if (m_specialization == Specialization::VANGUARD) ac += 2;
    if (m_specialization == Specialization::PALADIN) ac += 1;
    ac += getTalentRank(TalentNode::TACTICS);

    if (m_armor) ac += m_armor->getAcBonus();
    if (m_shield) ac += m_shield->getAcBonus();
    if (m_accessory) ac += m_accessory->getAcBonus();
    if (m_weapon) ac += m_weapon->getAffix().bonusAc;
    if (m_guarding) ac += 3;
    if (m_wardTurns > 0) ac += 2;
    if (m_frostTurns > 0) ac -= 1;
    return ac;
}

bool Character::canEquip(const Equipment& equipItem) const {
    const std::string id = equipItem.getId();
    const EquipSlot slot = equipItem.getSlot();

    if (slot == EquipSlot::SHIELD) {
        if (m_class == CharacterClass::MAGE || m_class == CharacterClass::ROGUE) return false;
        if (m_weapon && isTwoHandedWeaponId(m_weapon->getId())) return false;
    }
    if (slot == EquipSlot::ARMOR) {
        if (m_class == CharacterClass::MAGE && id != "arm_robe" && id != "arm_archmage_robe" &&
            id != "arm_void_mantle") {
            return false;
        }
        if (m_class == CharacterClass::ROGUE && isHeavyArmorId(id)) return false;
        if ((id == "arm_plate" || id == "arm_obsidian_plate" || id == "arm_dragonscale") &&
            getAbilities().strength < 13) {
            return false;
        }
    }
    if (slot == EquipSlot::WEAPON) {
        if (m_class == CharacterClass::MAGE &&
            id != "wpn_dagger" && id != "wpn_staff" && id != "wpn_archmage_staff" &&
            id != "wpn_frost_wand" && id != "wpn_flame_scepter") {
            return false;
        }
    }
    return true;
}

bool Character::equip(std::shared_ptr<Equipment> equipItem) {
    if (!equipItem || !canEquip(*equipItem)) return false;
    switch (equipItem->getSlot()) {
        case EquipSlot::WEAPON:
            m_weapon = std::move(equipItem);
            if (isTwoHandedWeaponId(m_weapon->getId())) m_shield.reset();
            break;
        case EquipSlot::ARMOR:
            m_armor = std::move(equipItem);
            break;
        case EquipSlot::SHIELD:
            m_shield = std::move(equipItem);
            break;
        case EquipSlot::ACCESSORY:
            m_accessory = std::move(equipItem);
            break;
    }
    recalculateDerivedMaximums(false);
    return true;
}

void Character::unequip(EquipSlot slot) {
    switch (slot) {
        case EquipSlot::WEAPON:    m_weapon.reset(); break;
        case EquipSlot::ARMOR:     m_armor.reset(); break;
        case EquipSlot::SHIELD:    m_shield.reset(); break;
        case EquipSlot::ACCESSORY: m_accessory.reset(); break;
    }
    recalculateDerivedMaximums(false);
}

int Character::getNextLevelXp() const {
    if (m_level >= MAX_LEVEL) return 0;
    return 100 * m_level * m_level;
}

bool Character::addXp(int amount) {
    if (amount <= 0) return false;
    m_xp += amount;
    bool leveledUp = false;
    while (m_level < MAX_LEVEL && m_xp >= getNextLevelXp()) {
        ++m_level;
        ++m_talentPoints;
        const int die = hitDieForClass(m_class);
        const int gain = std::max(die / 2 + 1, SessionRng::global().rollDie(die));
        m_baseHpRolls += gain;
        // 짝수 레벨마다 주 능력치 +1 자연 성장
        if (m_level % 2 == 0) {
            switch (m_class) {
                case CharacterClass::WARRIOR: m_abilities.strength = std::min(22, m_abilities.strength + 1); break;
                case CharacterClass::MAGE:    m_abilities.intelligence = std::min(22, m_abilities.intelligence + 1); break;
                case CharacterClass::ROGUE:   m_abilities.dexterity = std::min(22, m_abilities.dexterity + 1); break;
                case CharacterClass::CLERIC:  m_abilities.wisdom = std::min(22, m_abilities.wisdom + 1); break;
            }
        }
        recalculateDerivedMaximums(true);
        m_hp = m_maxHp;
        m_spellSlots = m_maxSpellSlots;
        initSkillsForLevel();
        leveledUp = true;
    }
    return leveledUp;
}

void Character::rest() {
    recalculateDerivedMaximums(false);
    m_hp = m_maxHp;
    m_spellSlots = m_maxSpellSlots;
    cureAllDebuffs();
    clearCombatBuffs();
}

void Character::sanctuaryRest() {
    if (isDead()) return;
    const int healAmt = std::max(4, static_cast<int>(m_maxHp * 0.60f));
    heal(healAmt);
    const int slotGain = std::max(1, m_maxSpellSlots / 2);
    recoverSpellSlot(slotGain);
    m_poisonTurns = 0;
    m_burnTurns = 0;
    m_bleedTurns = 0;
    m_frostTurns = 0;
}

void Character::heal(int amount) {
    if (isDead() || amount <= 0) return;
    m_hp = std::min(m_maxHp, m_hp + amount);
}

void Character::revive(int hpAmount) {
    if (!isDead()) {
        heal(hpAmount);
        return;
    }
    m_hp = std::clamp(hpAmount, 1, m_maxHp);
    cureAllDebuffs();
}

void Character::takeDamage(int damage) {
    if (damage <= 0) return;
    m_hp = std::max(0, m_hp - damage);
    if (m_hp == 0) {
        m_guarding = false;
        m_tauntTurns = 0;
    }
}

bool Character::isDead() const { return m_hp <= 0; }
std::string Character::getName() const { return m_name; }
int Character::getAge() const { return m_age; }
Gender Character::getGender() const { return m_gender; }
CharacterClass Character::getClass() const { return m_class; }

std::string Character::getClassString() const {
    const auto& lm = LocalizationManager::getInstance();
    switch (m_class) {
        case CharacterClass::WARRIOR: return lm.get("CLASS_WARRIOR");
        case CharacterClass::MAGE:    return lm.get("CLASS_MAGE");
        case CharacterClass::ROGUE:   return lm.get("CLASS_ROGUE");
        case CharacterClass::CLERIC:  return lm.get("CLASS_CLERIC");
    }
    return lm.get("CLASS_WARRIOR");
}

std::string Character::getClassSlug() const {
    switch (m_class) {
        case CharacterClass::WARRIOR: return "warrior";
        case CharacterClass::MAGE:    return "mage";
        case CharacterClass::ROGUE:   return "rogue";
        case CharacterClass::CLERIC:  return "cleric";
    }
    return "warrior";
}

int Character::getLevel() const { return m_level; }
int Character::getXp() const { return m_xp; }
int Character::getHp() const { return m_hp; }
int Character::getMaxHp() const { return m_maxHp; }
int Character::getSpellSlots() const { return m_spellSlots; }
int Character::getMaxSpellSlots() const { return m_maxSpellSlots; }
void Character::consumeSpellSlot() { if (m_spellSlots > 0) --m_spellSlots; }
void Character::recoverSpellSlot(int amount) {
    if (amount <= 0) return;
    m_spellSlots = std::min(m_maxSpellSlots, m_spellSlots + amount);
}

AbilityScore Character::getRawAbilities() const { return m_abilities; }

AbilityScore Character::getAbilities() const {
    AbilityScore eff = m_abilities;
    if (m_strBuffTurns > 0) eff.strength += m_strBuffAmount;
    if (m_dexBuffTurns > 0) eff.dexterity += m_dexBuffAmount;

    for (const auto& eq : {m_weapon, m_armor, m_shield, m_accessory}) {
        if (!eq) continue;
        eff.strength += eq->getStatBonus("str");
        eff.dexterity += eq->getStatBonus("dex");
        eff.constitution += eq->getStatBonus("con");
        eff.intelligence += eq->getStatBonus("int");
        eff.wisdom += eq->getStatBonus("wis");
        eff.charisma += eq->getStatBonus("cha");
    }
    return eff;
}

std::shared_ptr<Equipment> Character::getEquippedItem(EquipSlot slot) const {
    switch (slot) {
        case EquipSlot::WEAPON:    return m_weapon;
        case EquipSlot::ARMOR:     return m_armor;
        case EquipSlot::SHIELD:    return m_shield;
        case EquipSlot::ACCESSORY: return m_accessory;
    }
    return nullptr;
}

bool Character::canChooseSpecialization() const {
    return m_level >= SPEC_UNLOCK_LEVEL && m_specialization == Specialization::NONE;
}

std::pair<Specialization, Specialization> Character::getAvailableSpecializations() const {
    switch (m_class) {
        case CharacterClass::WARRIOR: return {Specialization::VANGUARD, Specialization::BERSERKER};
        case CharacterClass::MAGE:    return {Specialization::PYROMANCER, Specialization::CHRONOMANCER};
        case CharacterClass::ROGUE:   return {Specialization::ASSASSIN, Specialization::TRAPSMITH};
        case CharacterClass::CLERIC:  return {Specialization::PALADIN, Specialization::ORACLE};
    }
    return {Specialization::VANGUARD, Specialization::BERSERKER};
}

bool Character::chooseSpecialization(Specialization spec) {
    if (!canChooseSpecialization()) return false;
    const auto [a, b] = getAvailableSpecializations();
    if (spec != a && spec != b) return false;
    m_specialization = spec;
    recalculateDerivedMaximums(true);
    initSkillsForLevel();
    return true;
}

std::string Character::getSpecializationName() const {
    const auto& lm = LocalizationManager::getInstance();
    switch (m_specialization) {
        case Specialization::NONE:         return lm.get("SPEC_NONE");
        case Specialization::VANGUARD:     return lm.get("SPEC_VANGUARD");
        case Specialization::BERSERKER:    return lm.get("SPEC_BERSERKER");
        case Specialization::PYROMANCER:   return lm.get("SPEC_PYROMANCER");
        case Specialization::CHRONOMANCER: return lm.get("SPEC_CHRONOMANCER");
        case Specialization::ASSASSIN:     return lm.get("SPEC_ASSASSIN");
        case Specialization::TRAPSMITH:    return lm.get("SPEC_TRAPSMITH");
        case Specialization::PALADIN:      return lm.get("SPEC_PALADIN");
        case Specialization::ORACLE:       return lm.get("SPEC_ORACLE");
    }
    return lm.get("SPEC_NONE");
}

int Character::getTalentRank(TalentNode node) const {
    return m_talents[static_cast<std::size_t>(node)];
}

bool Character::investTalent(TalentNode node) {
    if (m_talentPoints <= 0) return false;
    const auto idx = static_cast<std::size_t>(node);
    if (m_talents[idx] >= MAX_TALENT_RANK) return false;
    m_talents[idx] += 1;
    m_talentPoints -= 1;
    recalculateDerivedMaximums(true);
    return true;
}

int Character::getCritThreshold() const {
    int thresh = 20;
    if (m_specialization == Specialization::ASSASSIN) thresh -= 1;
    int bonusPct = getTalentRank(TalentNode::APEX) * 3;
    for (const auto& eq : {m_weapon, m_armor, m_shield, m_accessory}) {
        if (eq) bonusPct += eq->getCritBonusPct();
    }
    thresh -= bonusPct / 5;
    return std::clamp(thresh, 16, 20);
}

int Character::getLifestealPct() const {
    int pct = (m_specialization == Specialization::BERSERKER) ? 10 : 0;
    for (const auto& eq : {m_weapon, m_armor, m_shield, m_accessory}) {
        if (eq) pct += eq->getLifestealPct();
    }
    return std::min(50, pct);
}

int Character::getStaggerBonus() const {
    return (getTalentRank(TalentNode::TACTICS) >= 2 ? 1 : 0) +
           (m_specialization == Specialization::VANGUARD ? 1 : 0);
}

void Character::setPoison(int turns) { m_poisonTurns = std::max(0, turns); }
void Character::setParalysis(int turns) { m_paralysisTurns = std::max(0, turns); }
void Character::setBurn(int turns) { m_burnTurns = std::max(0, turns); }
void Character::setBleed(int turns) { m_bleedTurns = std::max(0, turns); }
void Character::setFrost(int turns) { m_frostTurns = std::max(0, turns); }
int Character::getPoisonTurns() const { return m_poisonTurns; }
int Character::getParalysisTurns() const { return m_paralysisTurns; }
int Character::getBurnTurns() const { return m_burnTurns; }
int Character::getBleedTurns() const { return m_bleedTurns; }
int Character::getFrostTurns() const { return m_frostTurns; }
bool Character::isParalyzed() const { return m_paralysisTurns > 0; }
bool Character::hasAnyDebuff() const {
    return m_poisonTurns > 0 || m_paralysisTurns > 0 || m_burnTurns > 0 ||
           m_bleedTurns > 0 || m_frostTurns > 0;
}

void Character::cureAllDebuffs() {
    m_poisonTurns = 0;
    m_paralysisTurns = 0;
    m_burnTurns = 0;
    m_bleedTurns = 0;
    m_frostTurns = 0;
}

void Character::applyStrBuff(int amount, int turns) {
    m_strBuffAmount = std::max(m_strBuffAmount, amount);
    m_strBuffTurns = std::max(m_strBuffTurns, turns);
}

void Character::applyDexBuff(int amount, int turns) {
    m_dexBuffAmount = std::max(m_dexBuffAmount, amount);
    m_dexBuffTurns = std::max(m_dexBuffTurns, turns);
}

void Character::applyBless(int turns) {
    m_blessTurns = std::max(m_blessTurns, turns);
}

int Character::getStrBuffAmount() const { return m_strBuffAmount; }
int Character::getDexBuffAmount() const { return m_dexBuffAmount; }
int Character::getStrBuffTurns() const { return m_strBuffTurns; }
int Character::getDexBuffTurns() const { return m_dexBuffTurns; }
int Character::getBlessTurns() const { return m_blessTurns; }

void Character::processTurnEffects(std::vector<std::string>& logOutput) {
    if (isDead()) return;
    const auto& lm = LocalizationManager::getInstance();

    // 자신의 턴 시작 시 이전 턴의 방어 태세 종료
    m_guarding = false;

    if (m_poisonTurns > 0) {
        const int dmg = SessionRng::global().rollDie(4);
        takeDamage(dmg);
        logOutput.push_back(lm.format("STATUS_LOG_POISON_DAMAGE", {
            {"target", m_name}, {"damage", std::to_string(dmg)}}));
        --m_poisonTurns;
        if (isDead()) return;
    }
    if (m_burnTurns > 0) {
        const int dmg = SessionRng::global().rollDie(4) + 1;
        takeDamage(dmg);
        logOutput.push_back(lm.format("STATUS_LOG_BURN_DAMAGE", {
            {"target", m_name}, {"damage", std::to_string(dmg)}}));
        --m_burnTurns;
        if (isDead()) return;
    }
    if (m_bleedTurns > 0) {
        const int dmg = SessionRng::global().rollDie(3) + 1;
        takeDamage(dmg);
        logOutput.push_back(lm.format("STATUS_LOG_BLEED_DAMAGE", {
            {"target", m_name}, {"damage", std::to_string(dmg)}}));
        --m_bleedTurns;
        if (isDead()) return;
    }
    if (m_paralysisTurns > 0) --m_paralysisTurns;
    if (m_frostTurns > 0) --m_frostTurns;
}

void Character::advanceCombatBuffDurations(std::vector<std::string>& logOutput) {
    (void)logOutput;
    if (m_strBuffTurns > 0) {
        --m_strBuffTurns;
        if (m_strBuffTurns == 0) m_strBuffAmount = 0;
    }
    if (m_dexBuffTurns > 0) {
        --m_dexBuffTurns;
        if (m_dexBuffTurns == 0) m_dexBuffAmount = 0;
    }
    if (m_blessTurns > 0) --m_blessTurns;
    if (m_tauntTurns > 0) --m_tauntTurns;
    if (m_wardTurns > 0) --m_wardTurns;
}

void Character::clearCombatBuffs() {
    m_guarding = false;
    m_tauntTurns = 0;
    m_wardTurns = 0;
    m_strBuffAmount = 0;
    m_strBuffTurns = 0;
    m_dexBuffAmount = 0;
    m_dexBuffTurns = 0;
    m_blessTurns = 0;
    m_paralysisTurns = 0;
    m_frostTurns = 0;
    m_bleedTurns = 0;
    m_burnTurns = 0;
}

void Character::learnSkill(std::shared_ptr<Skill> skill) {
    if (!skill) return;
    for (const auto& existing : m_skills) {
        if (existing->getId() == skill->getId()) return;
    }
    m_skills.push_back(std::move(skill));
}

const std::vector<std::shared_ptr<Skill>>& Character::getSkills() const { return m_skills; }

void Character::initSkillsForLevel() {
    m_skills = SkillFactory::getSkillsForClassAndLevel(m_class, m_level, m_specialization);
}

nlohmann::json Character::toJson() const {
    nlohmann::json j;
    j["name"] = m_name;
    j["age"] = m_age;
    j["gender"] = static_cast<int>(m_gender);
    j["class"] = static_cast<int>(m_class);
    j["specialization"] = static_cast<int>(m_specialization);
    j["row"] = static_cast<int>(m_row);
    j["level"] = m_level;
    j["xp"] = m_xp;
    j["hp"] = m_hp;
    j["maxHp"] = m_maxHp;
    j["baseHpRolls"] = m_baseHpRolls;
    j["spellSlots"] = m_spellSlots;
    j["maxSpellSlots"] = m_maxSpellSlots;
    j["talentPoints"] = m_talentPoints;
    j["talents"] = m_talents;
    j["poisonTurns"] = m_poisonTurns;
    j["burnTurns"] = m_burnTurns;
    j["abilities"] = {
        {"str", m_abilities.strength},
        {"dex", m_abilities.dexterity},
        {"con", m_abilities.constitution},
        {"int", m_abilities.intelligence},
        {"wis", m_abilities.wisdom},
        {"cha", m_abilities.charisma}
    };
    j["equipment"] = {
        {"weapon", m_weapon ? m_weapon->toJson() : nlohmann::json(nullptr)},
        {"armor", m_armor ? m_armor->toJson() : nlohmann::json(nullptr)},
        {"shield", m_shield ? m_shield->toJson() : nlohmann::json(nullptr)},
        {"accessory", m_accessory ? m_accessory->toJson() : nlohmann::json(nullptr)}
    };
    return j;
}

std::unique_ptr<Character> Character::fromJson(const nlohmann::json& j) {
    if (!j.is_object()) throw std::runtime_error("character must be object");
    CharacterIdentity id;
    id.name = j.at("name").get<std::string>();
    if (!CharacterIdentityRules::isValidName(id.name)) throw std::runtime_error("invalid character name");
    id.age = j.at("age").get<int>();
    if (!CharacterIdentityRules::isValidAge(id.age)) throw std::runtime_error("invalid character age");
    const int genderVal = j.at("gender").get<int>();
    if (genderVal < 0 || genderVal > static_cast<int>(Gender::UNSPECIFIED)) {
        throw std::runtime_error("invalid gender");
    }
    id.gender = static_cast<Gender>(genderVal);
    const int classVal = j.at("class").get<int>();
    if (classVal < 0 || classVal > static_cast<int>(CharacterClass::CLERIC)) {
        throw std::runtime_error("invalid class");
    }
    id.characterClass = static_cast<CharacterClass>(classVal);

    const auto& ab = j.at("abilities");
    AbilityScore score;
    score.strength = ab.at("str").get<int>();
    score.dexterity = ab.at("dex").get<int>();
    score.constitution = ab.at("con").get<int>();
    score.intelligence = ab.at("int").get<int>();
    score.wisdom = ab.at("wis").get<int>();
    score.charisma = ab.at("cha").get<int>();

    auto ch = std::make_unique<Character>(id, score);
    ch->m_weapon.reset();
    ch->m_armor.reset();
    ch->m_shield.reset();
    ch->m_accessory.reset();

    ch->m_level = std::clamp(j.at("level").get<int>(), 1, MAX_LEVEL);
    ch->m_xp = std::max(0, j.at("xp").get<int>());
    ch->m_baseHpRolls = std::max(4, j.at("baseHpRolls").get<int>());
    const int specVal = j.value("specialization", 0);
    if (specVal < 0 || specVal > static_cast<int>(Specialization::ORACLE)) {
        throw std::runtime_error("invalid specialization");
    }
    ch->m_specialization = static_cast<Specialization>(specVal);
    const int rowVal = j.value("row", 0);
    if (rowVal < 0 || rowVal > 1) throw std::runtime_error("invalid row");
    ch->m_row = static_cast<CombatRow>(rowVal);

    ch->m_talentPoints = std::max(0, j.value("talentPoints", 0));
    if (j.contains("talents") && j["talents"].is_array() && j["talents"].size() == 4) {
        for (std::size_t i = 0; i < 4; ++i) {
            ch->m_talents[i] = std::clamp(j["talents"][i].get<int>(), 0, MAX_TALENT_RANK);
        }
    }

    if (j.contains("equipment") && j["equipment"].is_object()) {
        const auto& eq = j["equipment"];
        auto loadSlot = [&](const char* key) {
            if (!eq.contains(key) || eq[key].is_null()) return;
            auto item = ItemFactory::createItemFromJson(eq[key]);
            auto equipPtr = std::dynamic_pointer_cast<Equipment>(item);
            if (!equipPtr || !ch->equip(equipPtr)) {
                throw std::runtime_error(std::string("invalid equipped item in slot: ") + key);
            }
        };
        loadSlot("weapon");
        loadSlot("armor");
        loadSlot("shield");
        loadSlot("accessory");
    }

    ch->recalculateDerivedMaximums(false);
    ch->m_hp = std::clamp(j.at("hp").get<int>(), 0, ch->m_maxHp);
    ch->m_spellSlots = std::clamp(j.at("spellSlots").get<int>(), 0, ch->m_maxSpellSlots);
    ch->m_poisonTurns = std::max(0, j.value("poisonTurns", 0));
    ch->m_burnTurns = std::max(0, j.value("burnTurns", 0));
    ch->initSkillsForLevel();
    return ch;
}

} // namespace crawl
