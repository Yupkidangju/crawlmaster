#include "model/SkillFactory.hpp"
#include "model/ConcreteSkills.hpp"

namespace crawl {

std::shared_ptr<Skill> SkillFactory::createSkill(const std::string& id) {
    using T = SkillTargetType;
    using D = DamageType;

    // === 전사 (Warrior) ===
    if (id == "skl_slash") {
        SkillSpec s{id, 1, false, 0, T::SINGLE_FOE, D::SLASHING, true, false, false, 2, 1, 4, 2, 1};
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "skl_shield_bash") {
        SkillSpec s{id, 2, false, 0, T::SINGLE_FOE, D::BLUDGEONING, false, false, true, 2, 1, 6, 3, 3};
        s.inflictsParalysis = true;
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "skl_cleave") {
        SkillSpec s{id, 3, false, 0, T::ALL_FOES, D::SLASHING, true, false, false, 0, 1, 4, 1, 1};
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "skl_war_cry") {
        SkillSpec s{id, 4, false, 0, T::ALL_ALLIES, D::BLUDGEONING};
        s.appliesStrBuff = true;
        s.appliesTaunt = true;
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "skl_aegis_wall") {
        SkillSpec s{id, 3, false, 0, T::ALL_ALLIES, D::HOLY};
        s.appliesWard = true;
        s.appliesTaunt = true;
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "skl_blood_frenzy") {
        SkillSpec s{id, 3, false, 0, T::SINGLE_FOE, D::SLASHING, true, false, false, 1, 2, 6, 4, 2};
        s.inflictsBleed = true;
        s.extraLifesteal = true;
        return std::make_shared<TacticalSkill>(s);
    }

    // === 마법사 (Mage) ===
    if (id == "spl_magic_missile") {
        SkillSpec s{id, 1, true, 1, T::SINGLE_FOE, D::ARCANE, false, true, false, 0, 2, 4, 3, 1};
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "spl_frost_lance") {
        SkillSpec s{id, 2, true, 1, T::SINGLE_FOE, D::ICE, false, true, false, 0, 2, 6, 3, 2};
        s.inflictsFrost = true;
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "spl_fireball") {
        SkillSpec s{id, 3, true, 2, T::ALL_FOES, D::FIRE, false, true, false, 0, 3, 6, 2, 1};
        s.inflictsBurn = true;
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "spl_chain_lightning") {
        SkillSpec s{id, 4, true, 2, T::ALL_FOES, D::LIGHTNING, false, true, false, 0, 3, 6, 3, 2};
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "spl_meteor_spark") {
        SkillSpec s{id, 3, true, 2, T::SINGLE_FOE, D::FIRE, false, true, false, 0, 4, 8, 6, 3};
        s.inflictsBurn = true;
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "spl_time_fracture") {
        SkillSpec s{id, 3, true, 2, T::SINGLE_FOE, D::ARCANE, false, true, false, 0, 3, 6, 4, 3};
        s.inflictsParalysis = true;
        s.appliesBless = true;
        return std::make_shared<TacticalSkill>(s);
    }

    // === 도적 (Rogue) ===
    if (id == "skl_sneak_attack") {
        SkillSpec s{id, 1, false, 0, T::SINGLE_FOE, D::PIERCING, true, false, false, 3, 1, 6, 2, 1};
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "skl_poison_dart") {
        SkillSpec s{id, 2, false, 0, T::SINGLE_FOE, D::POISON, false, true, false, 2, 1, 6, 3, 1};
        s.inflictsPoison = true;
        s.inflictsBleed = true;
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "skl_shadowstrike") {
        SkillSpec s{id, 3, false, 0, T::SINGLE_FOE, D::PIERCING, true, false, false, 3, 2, 6, 3, 2};
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "skl_smoke_bomb") {
        SkillSpec s{id, 4, false, 0, T::ALL_ALLIES, D::POISON};
        s.appliesDexBuff = true;
        s.appliesWard = true;
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "skl_execute") {
        SkillSpec s{id, 3, false, 0, T::SINGLE_FOE, D::PIERCING, true, false, false, 4, 2, 6, 4, 2};
        s.bonusVsBrokenOrLowHp = true;
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "skl_acid_flask") {
        SkillSpec s{id, 3, false, 0, T::ALL_FOES, D::POISON, false, true, false, 0, 2, 6, 3, 2};
        s.inflictsPoison = true;
        return std::make_shared<TacticalSkill>(s);
    }

    // === 성직자 (Cleric) ===
    if (id == "spl_cure_wounds") {
        SkillSpec s{id, 1, true, 1, T::SINGLE_ALLY, D::HOLY, false, true, false, 0, 2, 8, 4, 0};
        s.isHeal = true;
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "spl_bless") {
        SkillSpec s{id, 2, true, 1, T::ALL_ALLIES, D::HOLY};
        s.appliesBless = true;
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "spl_smite") {
        SkillSpec s{id, 2, true, 1, T::SINGLE_FOE, D::HOLY, false, true, false, 0, 2, 8, 4, 3};
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "spl_prayer_of_healing") {
        SkillSpec s{id, 3, true, 2, T::ALL_ALLIES, D::HOLY, false, true, false, 0, 2, 6, 5, 0};
        s.isHeal = true;
        s.curesDebuffs = true;
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "spl_radiant_aegis") {
        SkillSpec s{id, 3, true, 2, T::ALL_FOES, D::HOLY, false, true, false, 0, 2, 8, 4, 2};
        s.appliesWard = true;
        return std::make_shared<TacticalSkill>(s);
    }
    if (id == "spl_divine_revival") {
        SkillSpec s{id, 3, true, 2, T::SINGLE_ALLY, D::HOLY, false, true, false, 0, 3, 8, 8, 0};
        s.isHeal = true;
        s.isRevive = true;
        s.curesDebuffs = true;
        return std::make_shared<TacticalSkill>(s);
    }

    return nullptr;
}

std::vector<std::string> SkillFactory::getRegisteredIds() {
    return {
        "skl_slash", "skl_shield_bash", "skl_cleave", "skl_war_cry", "skl_aegis_wall", "skl_blood_frenzy",
        "spl_magic_missile", "spl_frost_lance", "spl_fireball", "spl_chain_lightning", "spl_meteor_spark", "spl_time_fracture",
        "skl_sneak_attack", "skl_poison_dart", "skl_shadowstrike", "skl_smoke_bomb", "skl_execute", "skl_acid_flask",
        "spl_cure_wounds", "spl_bless", "spl_smite", "spl_prayer_of_healing", "spl_radiant_aegis", "spl_divine_revival"
    };
}

std::vector<std::shared_ptr<Skill>> SkillFactory::getSkillsForClassAndLevel(
    CharacterClass charClass, int level, Specialization spec) {
    std::vector<std::shared_ptr<Skill>> vec;
    auto add = [&](const char* id) {
        if (auto sk = createSkill(id)) vec.push_back(std::move(sk));
    };

    if (charClass == CharacterClass::WARRIOR) {
        if (level >= 1) add("skl_slash");
        if (level >= 2) add("skl_shield_bash");
        if (level >= 3) add("skl_cleave");
        if (level >= 4) add("skl_war_cry");
        if (spec == Specialization::VANGUARD) add("skl_aegis_wall");
        if (spec == Specialization::BERSERKER) add("skl_blood_frenzy");
    } else if (charClass == CharacterClass::MAGE) {
        if (level >= 1) add("spl_magic_missile");
        if (level >= 2) add("spl_frost_lance");
        if (level >= 3) add("spl_fireball");
        if (level >= 4) add("spl_chain_lightning");
        if (spec == Specialization::PYROMANCER) add("spl_meteor_spark");
        if (spec == Specialization::CHRONOMANCER) add("spl_time_fracture");
    } else if (charClass == CharacterClass::ROGUE) {
        if (level >= 1) add("skl_sneak_attack");
        if (level >= 2) add("skl_poison_dart");
        if (level >= 3) add("skl_shadowstrike");
        if (level >= 4) add("skl_smoke_bomb");
        if (spec == Specialization::ASSASSIN) add("skl_execute");
        if (spec == Specialization::TRAPSMITH) add("skl_acid_flask");
    } else if (charClass == CharacterClass::CLERIC) {
        if (level >= 1) add("spl_cure_wounds");
        if (level >= 2) {
            add("spl_bless");
            add("spl_smite");
        }
        if (level >= 3) add("spl_prayer_of_healing");
        if (spec == Specialization::PALADIN) add("spl_radiant_aegis");
        if (spec == Specialization::ORACLE) add("spl_divine_revival");
    }
    return vec;
}

} // namespace crawl
