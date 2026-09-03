// [v0.8.0] SkillFactory.cpp 신규 작성
// 스킬 ID 매핑 생성 팩토리 로직 구현.

#include "model/SkillFactory.hpp"
#include "model/ConcreteSkills.hpp"
#include "model/Character.hpp" // CharacterClass 참조용

namespace crawl {

std::shared_ptr<Skill> SkillFactory::createSkill(const std::string& id) {
    if (id == "skl_slash") {
        return std::make_shared<SlashSkill>();
    } else if (id == "skl_shield_bash") {
        return std::make_shared<ShieldBashSkill>();
    } else if (id == "skl_cleave") {
        return std::make_shared<CleaveSkill>();
    } else if (id == "spl_magic_missile") {
        return std::make_shared<MagicMissileSpell>();
    } else if (id == "spl_sleep") {
        return std::make_shared<SleepSpell>();
    } else if (id == "spl_fireball") {
        return std::make_shared<FireballSpell>();
    } else if (id == "skl_sneak_attack") {
        return std::make_shared<SneakAttackSkill>();
    } else if (id == "skl_poison_dart") {
        return std::make_shared<PoisonDartSkill>();
    } else if (id == "skl_shadowstrike") {
        return std::make_shared<ShadowstrikeSkill>();
    } else if (id == "spl_cure_wounds") {
        return std::make_shared<CureWoundsSpell>();
    } else if (id == "spl_bless") {
        return std::make_shared<BlessSpell>();
    } else if (id == "spl_prayer_of_healing") {
        return std::make_shared<PrayerOfHealingSpell>();
    }
    return nullptr;
}

std::vector<std::string> SkillFactory::getRegisteredIds() {
    return {
        "skl_slash", "skl_shield_bash", "skl_cleave",
        "spl_magic_missile", "spl_sleep", "spl_fireball",
        "skl_sneak_attack", "skl_poison_dart", "skl_shadowstrike",
        "spl_cure_wounds", "spl_bless", "spl_prayer_of_healing"
    };
}

std::vector<std::shared_ptr<Skill>> SkillFactory::getSkillsForClassAndLevel(CharacterClass charClass, int level) {
    std::vector<std::shared_ptr<Skill>> vec;
    
    // 전사 스킬 배포
    if (charClass == CharacterClass::WARRIOR) {
        if (level >= 1) vec.push_back(createSkill("skl_slash"));
        if (level >= 2) vec.push_back(createSkill("skl_shield_bash"));
        if (level >= 3) vec.push_back(createSkill("skl_cleave"));
    }
    // 마법사 주문 배포
    else if (charClass == CharacterClass::MAGE) {
        if (level >= 1) vec.push_back(createSkill("spl_magic_missile"));
        if (level >= 2) vec.push_back(createSkill("spl_sleep"));
        if (level >= 3) vec.push_back(createSkill("spl_fireball"));
    }
    // 도적 스킬 배포
    else if (charClass == CharacterClass::ROGUE) {
        if (level >= 1) vec.push_back(createSkill("skl_sneak_attack"));
        if (level >= 2) vec.push_back(createSkill("skl_poison_dart"));
        if (level >= 3) vec.push_back(createSkill("skl_shadowstrike"));
    }
    // 성직자 주문 배포
    else if (charClass == CharacterClass::CLERIC) {
        if (level >= 1) vec.push_back(createSkill("spl_cure_wounds"));
        if (level >= 2) vec.push_back(createSkill("spl_bless"));
        if (level >= 3) vec.push_back(createSkill("spl_prayer_of_healing"));
    }

    return vec;
}

} // namespace crawl
