// [v0.3.0] Character.cpp 신규 작성
// [v0.8.0] 상태이상(독, 마비), 전투 임시 버프(STR/DEX, 축복), 지팡이 지능 보너스 및 SkillFactory 연계 습득 스킬 자동 초기화 탑재.
// 4d6 Drop-Lowest 능력치 롤러, D&D 5e 중갑/경갑 AC 방어구 규정 적용, 레벨업 최대 HP 성장 및 JSON 영속화를 구현한다.

#include "model/Character.hpp"
#include "model/ItemFactory.hpp"
#include "model/SkillFactory.hpp"
#include "core/SessionRng.hpp"
#include "core/LocalizationManager.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace crawl {

Character::Character(std::string name, CharacterClass charClass)
    : Character(std::move(name), charClass, true) {}

Character::Character(std::string name, CharacterClass charClass, bool initializeRandomState)
    : m_name(std::move(name)), m_class(charClass), m_level(1), m_xp(0),
      m_hp(10), m_maxHp(10), m_spellSlots(0), m_maxSpellSlots(0),
      m_poisonTurns(0), m_paralysisTurns(0),
      m_strBuffTurns(0), m_strBuffAmount(0),
      m_dexBuffTurns(0), m_dexBuffAmount(0),
      m_blessTurns(0) {
    if (!initializeRandomState) return;
    // 1. 주사위 롤링으로 6대 능력치 초기화
    rollAbilities();

    // 2. 클래스별 기본 HP 및 스펠 슬롯 셋업 (D&D 5e 기준)
    int conMod = getAbilities().getModifier(m_abilities.constitution);
    int hitDie = getHitDieSides();
    m_maxHp = std::max(4, hitDie + conMod); // 최소 1레벨 HP는 4 이상 보장
    m_hp = m_maxHp;

    if (m_class == CharacterClass::MAGE || m_class == CharacterClass::CLERIC) {
        m_maxSpellSlots = 2; // 1레벨 주문 슬롯 2개 지급
        m_spellSlots = m_maxSpellSlots;
    }

    // 3. 클래스별 D&D 초기 기본 장비 지급 및 자동 장착
    if (m_class == CharacterClass::WARRIOR) {
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("wpn_longsword")));
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("arm_scale")));
    } else if (m_class == CharacterClass::MAGE) {
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("wpn_staff")));
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("arm_robe")));
    } else if (m_class == CharacterClass::ROGUE) {
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("wpn_dagger")));
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("arm_leather")));
    } else if (m_class == CharacterClass::CLERIC) {
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("wpn_mace")));
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("arm_chain")));
        equip(std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem("shd_round")));
    }

    // 4. [v0.8.0] 현재 레벨에 따른 스킬 초기 배포 습득
    initSkillsForLevel();
}

void Character::rollAbilities() {
    auto rollOneStat = [&]() -> int {
        int rolls[4];
        for (int i = 0; i < 4; ++i) {
            rolls[i] = SessionRng::global().rollDie(6);
        }
        std::sort(rolls, rolls + 4);
        // 최저값 rolls[0]을 버리고 상위 3개 합산 (4d6 drop lowest)
        return rolls[1] + rolls[2] + rolls[3];
    };

    m_abilities.strength = rollOneStat();
    m_abilities.dexterity = rollOneStat();
    m_abilities.constitution = rollOneStat();
    m_abilities.intelligence = rollOneStat();
    m_abilities.wisdom = rollOneStat();
    m_abilities.charisma = rollOneStat();
}

int Character::getAc() const {
    // 민첩 버프 수치가 실시간 AC에 영향을 주도록 getAbilities()로 DEX 획득
    int dexMod = getAbilities().getModifier(getAbilities().dexterity);
    int baseAc = 10 + dexMod;

    // 갑옷 장착에 따른 AC 산정 (D&D 5e 공식 구현)
    if (m_equippedArmor) {
        std::string armorId = m_equippedArmor->getId();
        if (armorId == "arm_chain") {
            // 체인 메일: 민첩 보정치 없는 고정 AC 16
            baseAc = 16;
        } else if (armorId == "arm_plate") {
            // 플레이트 아머: 민첩 보정치 없는 고정 AC 18
            baseAc = 18;
        } else if (armorId == "arm_scale") {
            // 스케일 메일: AC 14 + 민첩 보정치 (최대 +2)
            baseAc = 14 + std::min(2, dexMod);
        } else if (armorId == "arm_leather") {
            // 가죽 갑옷: AC 11 + 민첩 보정치
            baseAc = 11 + dexMod;
        } else {
            // 일반 의복/로브: 기본 AC + 갑옷 보너스 + 민첩 보정
            baseAc = 10 + dexMod + m_equippedArmor->getAcBonus();
        }
    }

    // 방패 추가 AC 보너스 반영
    if (m_equippedShield) {
        baseAc += m_equippedShield->getAcBonus();
    }

    return baseAc;
}

bool Character::canEquip(const Equipment& equipItem) const {
    const std::string itemId = equipItem.getId();
    if (m_class == CharacterClass::MAGE &&
        itemId != "wpn_dagger" && itemId != "wpn_staff" && itemId != "arm_robe") return false;
    if (m_class == CharacterClass::ROGUE &&
        itemId != "wpn_dagger" && itemId != "wpn_rapier" && itemId != "arm_leather") return false;
    if (m_class == CharacterClass::CLERIC &&
        (itemId == "wpn_greatsword" || itemId == "wpn_staff" || itemId == "wpn_rapier" ||
         itemId == "arm_plate" || itemId == "shd_tower")) return false;
    if (m_class == CharacterClass::WARRIOR && (itemId == "wpn_staff" || itemId == "arm_robe")) return false;

    const int strength = m_abilities.strength;
    if (itemId == "arm_plate" && strength < 15) return false;
    if (itemId == "shd_tower" && strength < 14) return false;
    if (itemId == "wpn_greatsword" && m_equippedShield) return false;
    if (equipItem.getSlot() == EquipSlot::SHIELD && m_equippedWeapon &&
        m_equippedWeapon->getId() == "wpn_greatsword") return false;
    return true;
}

bool Character::equip(std::shared_ptr<Equipment> equipItem) {
    if (!equipItem || !canEquip(*equipItem)) return false;

    EquipSlot slot = equipItem->getSlot();
    switch (slot) {
        case EquipSlot::WEAPON:
            m_equippedWeapon = equipItem;
            break;
        case EquipSlot::ARMOR:
            m_equippedArmor = equipItem;
            break;
        case EquipSlot::SHIELD:
            m_equippedShield = equipItem;
            break;
        default:
            return false;
    }
    return true;
}

void Character::unequip(EquipSlot slot) {
    switch (slot) {
        case EquipSlot::WEAPON: m_equippedWeapon = nullptr; break;
        case EquipSlot::ARMOR:  m_equippedArmor = nullptr;  break;
        case EquipSlot::SHIELD: m_equippedShield = nullptr; break;
        default: break;
    }
}

bool Character::addXp(int amount) {
    if (m_level >= 3) return false; // 최대 3레벨 제한

    m_xp += amount;
    int nextXpThreshold = (m_level == 1) ? 300 : 900;

    if (m_xp >= nextXpThreshold) {
        m_level++;
        
        // 레벨업에 따른 HP 상승 (주사위 굴림 + CON 보정치)
        int conMod = getAbilities().getModifier(m_abilities.constitution);
        int hpGain = std::max(1, SessionRng::global().rollDie(getHitDieSides()) + conMod);
        
        m_maxHp += hpGain;
        m_hp = m_maxHp; // 레벨업 시 HP 완전 치유

        // 주문 슬롯 획득 증가 처리
        if (m_class == CharacterClass::MAGE || m_class == CharacterClass::CLERIC) {
            m_maxSpellSlots += 1;
            m_spellSlots = m_maxSpellSlots;
        }

        // [v0.8.0] 레벨에 맞춰 새로운 스킬 풀 자동 습득 동기화
        initSkillsForLevel();

        std::cout << "[LevelUp] " << m_name << " 캐릭터가 " << m_level << " 레벨로 상승했습니다! (HP +" << hpGain << ")" << std::endl;
        return true;
    }
    return false;
}

void Character::rest() {
    m_hp = m_maxHp;
    m_spellSlots = m_maxSpellSlots;
    m_poisonTurns = 0;
    m_paralysisTurns = 0;
    clearCombatBuffs();
}

void Character::heal(int amount) {
    m_hp = std::min(m_maxHp, m_hp + amount);
}

void Character::takeDamage(int damage) {
    m_hp = std::max(0, m_hp - damage);
}

bool Character::isDead() const {
    return m_hp <= 0;
}

std::string Character::getName() const { return m_name; }
CharacterClass Character::getClass() const { return m_class; }

std::string Character::getClassString() const {
    switch (m_class) {
        case CharacterClass::WARRIOR: return "전사";
        case CharacterClass::MAGE:    return "마법사";
        case CharacterClass::ROGUE:   return "도적";
        case CharacterClass::CLERIC:  return "성직자";
    }
    return "미정";
}

int Character::getLevel() const { return m_level; }
int Character::getXp() const { return m_xp; }
int Character::getHp() const { return m_hp; }
int Character::getMaxHp() const { return m_maxHp; }
int Character::getSpellSlots() const { return m_spellSlots; }
int Character::getMaxSpellSlots() const { return m_maxSpellSlots; }
void Character::consumeSpellSlot() { m_spellSlots = std::max(0, m_spellSlots - 1); }
void Character::recoverSpellSlot(int amount) {
    m_spellSlots = std::min(m_maxSpellSlots, m_spellSlots + amount);
}

AbilityScore Character::getAbilities() const {
    AbilityScore copy = m_abilities;
    // 임시 힘 버프 적용
    if (m_strBuffTurns > 0) {
        copy.strength += m_strBuffAmount;
    }
    // 임시 민첩 버프 적용
    if (m_dexBuffTurns > 0) {
        copy.dexterity += m_dexBuffAmount;
    }
    // wpn_staff 장착 시 지능 +1 보너스
    if (m_equippedWeapon && m_equippedWeapon->getId() == "wpn_staff") {
        copy.intelligence += 1;
    }
    return copy;
}

AbilityScore Character::getRawAbilities() const {
    return m_abilities;
}

std::shared_ptr<Equipment> Character::getEquippedItem(EquipSlot slot) const {
    switch (slot) {
        case EquipSlot::WEAPON: return m_equippedWeapon;
        case EquipSlot::ARMOR:  return m_equippedArmor;
        case EquipSlot::SHIELD: return m_equippedShield;
        default: return nullptr;
    }
}

// --- [v0.8.0] 상태이상 및 버프 제어 함수 구현 ---

void Character::setPoison(int turns) {
    m_poisonTurns = turns;
}

void Character::setParalysis(int turns) {
    m_paralysisTurns = turns;
}

int Character::getPoisonTurns() const {
    return m_poisonTurns;
}

int Character::getParalysisTurns() const {
    return m_paralysisTurns;
}

bool Character::isParalyzed() const {
    return m_paralysisTurns > 0;
}

void Character::applyStrBuff(int amount, int turns) {
    m_strBuffAmount = amount;
    m_strBuffTurns = turns;
}

void Character::applyDexBuff(int amount, int turns) {
    m_dexBuffAmount = amount;
    m_dexBuffTurns = turns;
}

void Character::applyBless(int turns) {
    m_blessTurns = turns;
}

int Character::getStrBuffAmount() const {
    return m_strBuffTurns > 0 ? m_strBuffAmount : 0;
}

int Character::getDexBuffAmount() const {
    return m_dexBuffTurns > 0 ? m_dexBuffAmount : 0;
}

int Character::getBlessTurns() const {
    return m_blessTurns;
}

void Character::processTurnEffects(std::vector<std::string>& logOutput) {
    if (isDead()) return;

    // 독 대미지 정산 (매 턴 1d3의 지속 대미지 적용)
    if (m_poisonTurns > 0) {
        int poisonDmg = SessionRng::global().rollDie(3);
        takeDamage(poisonDmg);
        logOutput.push_back(LocalizationManager::getInstance().format("STATUS_LOG_POISON_DAMAGE", {
            {"target", m_name}, {"damage", std::to_string(poisonDmg)}}));
        
        m_poisonTurns = std::max(0, m_poisonTurns - 1);
        if (isDead()) {
            logOutput.push_back(LocalizationManager::getInstance().format("COMBAT_LOG_DEFEATED", {
                {"target", m_name}}));
        }
    }

    // 마비 및 기절 지속 턴수 감소
    if (m_paralysisTurns > 0) {
        m_paralysisTurns = std::max(0, m_paralysisTurns - 1);
    }

}

void Character::advanceCombatBuffDurations(std::vector<std::string>& logOutput) {
    if (m_strBuffTurns > 0) {
        m_strBuffTurns = std::max(0, m_strBuffTurns - 1);
        if (m_strBuffTurns == 0) {
            m_strBuffAmount = 0;
            logOutput.push_back(LocalizationManager::getInstance().format("STATUS_LOG_BUFF_EXPIRED", {
                {"target", m_name}, {"buff", LocalizationManager::getInstance().get("STATUS_STRENGTH")}}));
        }
    }
    if (m_dexBuffTurns > 0) {
        m_dexBuffTurns = std::max(0, m_dexBuffTurns - 1);
        if (m_dexBuffTurns == 0) {
            m_dexBuffAmount = 0;
            logOutput.push_back(LocalizationManager::getInstance().format("STATUS_LOG_BUFF_EXPIRED", {
                {"target", m_name}, {"buff", LocalizationManager::getInstance().get("STATUS_DEXTERITY")}}));
        }
    }
    if (m_blessTurns > 0) {
        m_blessTurns = std::max(0, m_blessTurns - 1);
        if (m_blessTurns == 0) {
            logOutput.push_back(LocalizationManager::getInstance().format("STATUS_LOG_BUFF_EXPIRED", {
                {"target", m_name}, {"buff", LocalizationManager::getInstance().get("STATUS_BLESS")}}));
        }
    }
}

void Character::clearCombatBuffs() {
    m_strBuffTurns = 0;
    m_strBuffAmount = 0;
    m_dexBuffTurns = 0;
    m_dexBuffAmount = 0;
    m_blessTurns = 0;
}

// --- [v0.8.0] 보유 스킬 제어 함수 구현 ---

void Character::learnSkill(std::shared_ptr<Skill> skill) {
    if (!skill) return;
    // 중복 확인
    auto it = std::find_if(m_skills.begin(), m_skills.end(), [&](const auto& s) {
        return s->getId() == skill->getId();
    });
    if (it == m_skills.end()) {
        m_skills.push_back(skill);
    }
}

const std::vector<std::shared_ptr<Skill>>& Character::getSkills() const {
    return m_skills;
}

void Character::initSkillsForLevel() {
    m_skills = SkillFactory::getSkillsForClassAndLevel(m_class, m_level);
}


int Character::getHitDieSides() const {
    switch (m_class) {
        case CharacterClass::WARRIOR: return 10;
        case CharacterClass::MAGE:    return 6;
        case CharacterClass::ROGUE:   return 8;
        case CharacterClass::CLERIC:  return 8;
    }
    return 6;
}

nlohmann::json Character::toJson() const {
    nlohmann::json j;
    j["name"] = m_name;
    j["class"] = static_cast<int>(m_class);
    j["level"] = m_level;
    j["xp"] = m_xp;
    j["hp"] = m_hp;
    j["maxHp"] = m_maxHp;
    j["spellSlots"] = m_spellSlots;
    j["maxSpellSlots"] = m_maxSpellSlots;

    // [v0.8.0] 독 및 마비 상태이상 영속 데이터 추가
    j["poisonTurns"] = m_poisonTurns;
    j["paralysisTurns"] = m_paralysisTurns;

    // 능력치 데이터 직렬화
    j["abilities"] = {
        {"strength", m_abilities.strength},
        {"dexterity", m_abilities.dexterity},
        {"constitution", m_abilities.constitution},
        {"intelligence", m_abilities.intelligence},
        {"wisdom", m_abilities.wisdom},
        {"charisma", m_abilities.charisma}
    };

    j["equipment"] = {
        {"weapon", m_equippedWeapon ? m_equippedWeapon->getId() : ""},
        {"armor", m_equippedArmor ? m_equippedArmor->getId() : ""},
        {"shield", m_equippedShield ? m_equippedShield->getId() : ""}
    };

    return j;
}

std::unique_ptr<Character> Character::fromJson(const nlohmann::json& j, int schemaVersion) {
    if (!j.is_object()) throw std::runtime_error("character는 객체여야 합니다.");

    const std::string name = j.at("name").get<std::string>();
    const int classValue = j.at("class").get<int>();
    const int level = j.at("level").get<int>();
    const int xp = j.at("xp").get<int>();
    const int hp = j.at("hp").get<int>();
    const bool canonical = schemaVersion >= 2;
    const int maxHp = j.at(canonical ? "maxHp" : "max_hp").get<int>();
    const int spellSlots = j.at(canonical ? "spellSlots" : "spell_slots").get<int>();
    const int maxSpellSlots = j.at(canonical ? "maxSpellSlots" : "max_spell_slots").get<int>();
    const int poisonTurns = j.value(canonical ? "poisonTurns" : "poison_turns", 0);
    const int paralysisTurns = j.value(canonical ? "paralysisTurns" : "paralysis_turns", 0);

    if (name.empty() || name.size() > 64) throw std::runtime_error("character name 길이가 잘못됐습니다.");
    if (classValue < 0 || classValue > 3) throw std::runtime_error("character class가 잘못됐습니다.");
    if (level < 1 || level > 3) throw std::runtime_error("character level 범위를 벗어났습니다.");
    if (xp < 0 || xp > 1'000'000'000) throw std::runtime_error("character xp 범위를 벗어났습니다.");
    if (maxHp < 1 || maxHp > 10'000 || hp < 0 || hp > maxHp) {
        throw std::runtime_error("character hp 범위를 벗어났습니다.");
    }
    if (maxSpellSlots < 0 || maxSpellSlots > 100 || spellSlots < 0 || spellSlots > maxSpellSlots) {
        throw std::runtime_error("character spell slot 범위를 벗어났습니다.");
    }
    if (poisonTurns < 0 || poisonTurns > 1'000 || paralysisTurns < 0 || paralysisTurns > 1'000) {
        throw std::runtime_error("character status turn 범위를 벗어났습니다.");
    }

    auto charClass = static_cast<CharacterClass>(classValue);
    auto character = std::unique_ptr<Character>(new Character(name, charClass, false));
    character->m_level = level;
    character->m_xp = xp;
    character->m_hp = hp;
    character->m_maxHp = maxHp;
    character->m_spellSlots = spellSlots;
    character->m_maxSpellSlots = maxSpellSlots;
    character->m_poisonTurns = poisonTurns;
    character->m_paralysisTurns = paralysisTurns;

    const auto& abilities = j.at("abilities");
    auto ability = [&](const char* canonicalKey, const char* legacyKey) {
        const int score = abilities.at(canonical ? canonicalKey : legacyKey).get<int>();
        if (score < 1 || score > 30) throw std::runtime_error("ability score 범위를 벗어났습니다.");
        return score;
    };
    character->m_abilities.strength = ability("strength", "str");
    character->m_abilities.dexterity = ability("dexterity", "dex");
    character->m_abilities.constitution = ability("constitution", "con");
    character->m_abilities.intelligence = ability("intelligence", "int");
    character->m_abilities.wisdom = ability("wisdom", "wis");
    character->m_abilities.charisma = ability("charisma", "cha");

    std::string wpnId;
    std::string armId;
    std::string shdId;
    if (canonical) {
        const auto& equipment = j.at("equipment");
        wpnId = equipment.at("weapon").get<std::string>();
        armId = equipment.at("armor").get<std::string>();
        shdId = equipment.at("shield").get<std::string>();
    } else {
        wpnId = j.at("eq_weapon").get<std::string>();
        armId = j.at("eq_armor").get<std::string>();
        shdId = j.at("eq_shield").get<std::string>();
    }

    auto equipmentById = [](const std::string& id) -> std::shared_ptr<Equipment> {
        if (id.empty()) return nullptr;
        auto equipment = std::dynamic_pointer_cast<Equipment>(ItemFactory::createItem(id));
        if (!equipment) throw std::runtime_error("알 수 없는 equipment id: " + id);
        return equipment;
    };

    character->unequip(EquipSlot::WEAPON);
    character->unequip(EquipSlot::ARMOR);
    character->unequip(EquipSlot::SHIELD);

    if (!wpnId.empty()) {
        const auto weapon = equipmentById(wpnId);
        if (weapon->getSlot() != EquipSlot::WEAPON || !character->equip(weapon)) {
            throw std::runtime_error("장착할 수 없는 weapon 조합입니다: " + wpnId);
        }
    } else {
        character->unequip(EquipSlot::WEAPON);
    }
    
    if (!armId.empty()) {
        const auto armor = equipmentById(armId);
        if (armor->getSlot() != EquipSlot::ARMOR || !character->equip(armor)) {
            throw std::runtime_error("장착할 수 없는 armor 조합입니다: " + armId);
        }
    } else {
        character->unequip(EquipSlot::ARMOR);
    }

    if (!shdId.empty()) {
        const auto shield = equipmentById(shdId);
        if (shield->getSlot() != EquipSlot::SHIELD || !character->equip(shield)) {
            throw std::runtime_error("장착할 수 없는 shield 조합입니다: " + shdId);
        }
    } else {
        character->unequip(EquipSlot::SHIELD);
    }

    // 레벨 복원에 따른 스킬 재로딩
    character->initSkillsForLevel();

    return character;
}

} // namespace crawl
