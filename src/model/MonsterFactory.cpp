#include "model/MonsterFactory.hpp"
#include "core/SessionRng.hpp"

#include <algorithm>

namespace crawl {

std::shared_ptr<Monster> MonsterFactory::createMonster(const std::string& id) {
    using D = DamageType;
    // === Floor 1: Sunken Catacombs ===
    if (id == "mon_kobold") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_KOBOLD_NAME", 14, 11, 35, 1, 5, 1, 1, 2,
            std::vector<D>{D::SLASHING, D::ICE}, std::vector<D>{}, false, D::PIERCING);
    }
    if (id == "mon_goblin") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_GOBLIN_NAME", 18, 12, 48, 1, 6, 2, 1, 2,
            std::vector<D>{D::FIRE, D::PIERCING}, std::vector<D>{}, false, D::SLASHING);
    }
    if (id == "mon_skeleton") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_SKELETON_NAME", 20, 12, 55, 1, 6, 2, 1, 3,
            std::vector<D>{D::BLUDGEONING, D::HOLY}, std::vector<D>{D::PIERCING, D::POISON}, false, D::SLASHING);
    }
    if (id == "mon_crypt_rat") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_CRYPT_RAT_NAME", 12, 11, 30, 1, 4, 2, 1, 2,
            std::vector<D>{D::FIRE, D::SLASHING}, std::vector<D>{D::POISON}, false, D::POISON);
    }
    if (id == "mon_crypt_warden") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_CRYPT_WARDEN_NAME", 64, 14, 220, 2, 6, 3, 2, 5,
            std::vector<D>{D::HOLY, D::BLUDGEONING, D::FIRE}, std::vector<D>{D::POISON, D::ICE}, true, D::SLASHING);
    }

    // === Floor 2: Drowned Cistern ===
    if (id == "mon_giant_spider") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_GIANT_SPIDER_NAME", 28, 13, 80, 1, 8, 2, 2, 3,
            std::vector<D>{D::FIRE, D::SLASHING}, std::vector<D>{D::POISON}, false, D::POISON);
    }
    if (id == "mon_goblin_shaman") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_GOBLIN_SHAMAN_NAME", 24, 12, 95, 1, 6, 3, 2, 2,
            std::vector<D>{D::PIERCING, D::SLASHING}, std::vector<D>{D::ARCANE}, false, D::ARCANE);
    }
    if (id == "mon_drowned_knight") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_DROWNED_KNIGHT_NAME", 36, 14, 115, 1, 10, 3, 2, 4,
            std::vector<D>{D::LIGHTNING, D::HOLY}, std::vector<D>{D::ICE, D::SLASHING}, false, D::ICE);
    }
    if (id == "mon_mire_slime") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_MIRE_SLIME_NAME", 32, 11, 90, 1, 8, 2, 2, 3,
            std::vector<D>{D::FIRE, D::LIGHTNING}, std::vector<D>{D::BLUDGEONING, D::POISON}, false, D::POISON);
    }
    if (id == "mon_bog_hydra") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_BOG_HYDRA_NAME", 96, 14, 340, 2, 8, 4, 3, 5,
            std::vector<D>{D::FIRE, D::LIGHTNING}, std::vector<D>{D::POISON, D::ICE}, true, D::POISON);
    }

    // === Floor 3: Cursed Ossuary ===
    if (id == "mon_ghoul") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_GHOUL_NAME", 40, 13, 130, 2, 6, 3, 3, 3,
            std::vector<D>{D::HOLY, D::FIRE}, std::vector<D>{D::POISON}, false, D::SLASHING);
    }
    if (id == "mon_orc") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_ORC_NAME", 48, 14, 145, 2, 6, 4, 3, 4,
            std::vector<D>{D::ICE, D::ARCANE}, std::vector<D>{D::BLUDGEONING}, false, D::SLASHING);
    }
    if (id == "mon_bone_archer") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_BONE_ARCHER_NAME", 34, 13, 135, 2, 6, 3, 3, 3,
            std::vector<D>{D::BLUDGEONING, D::HOLY}, std::vector<D>{D::PIERCING}, false, D::PIERCING);
    }
    if (id == "mon_wraith") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_WRAITH_NAME", 38, 15, 160, 2, 6, 4, 3, 3,
            std::vector<D>{D::HOLY, D::ARCANE, D::FIRE}, std::vector<D>{D::SLASHING, D::PIERCING}, false, D::ARCANE);
    }
    if (id == "mon_ossuary_lich") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_OSSUARY_LICH_NAME", 128, 15, 480, 2, 10, 5, 4, 5,
            std::vector<D>{D::HOLY, D::FIRE, D::BLUDGEONING}, std::vector<D>{D::ICE, D::POISON, D::ARCANE}, true, D::ARCANE);
    }

    // === Floor 4: Obsidian Foundry ===
    if (id == "mon_orc_berserker") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_ORC_BERSERKER_NAME", 62, 14, 210, 2, 8, 5, 4, 4,
            std::vector<D>{D::ICE, D::PIERCING}, std::vector<D>{D::FIRE}, false, D::SLASHING);
    }
    if (id == "mon_ember_golem") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_EMBER_GOLEM_NAME", 74, 16, 240, 2, 10, 5, 4, 5,
            std::vector<D>{D::ICE, D::BLUDGEONING}, std::vector<D>{D::FIRE, D::SLASHING, D::POISON}, false, D::FIRE);
    }
    if (id == "mon_ash_cultist") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_ASH_CULTIST_NAME", 50, 14, 200, 2, 8, 4, 4, 3,
            std::vector<D>{D::HOLY, D::ICE}, std::vector<D>{D::FIRE}, false, D::FIRE);
    }
    if (id == "mon_hellhound") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_HELLHOUND_NAME", 54, 15, 215, 2, 8, 5, 4, 3,
            std::vector<D>{D::ICE, D::HOLY}, std::vector<D>{D::FIRE}, false, D::FIRE);
    }
    if (id == "mon_iron_colossus") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_IRON_COLOSSUS_NAME", 165, 17, 650, 3, 8, 6, 5, 6,
            std::vector<D>{D::LIGHTNING, D::ICE}, std::vector<D>{D::SLASHING, D::PIERCING, D::FIRE, D::POISON}, true, D::BLUDGEONING);
    }

    // === Floor 5: Dragon's Sanctum ===
    if (id == "mon_drake_guard") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_DRAKE_GUARD_NAME", 82, 16, 290, 2, 10, 6, 5, 4,
            std::vector<D>{D::ICE, D::LIGHTNING}, std::vector<D>{D::FIRE}, false, D::PIERCING);
    }
    if (id == "mon_dragon_whelp") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_DRAGON_WHELP_NAME", 78, 15, 310, 2, 10, 6, 5, 4,
            std::vector<D>{D::ICE, D::PIERCING}, std::vector<D>{D::FIRE}, false, D::FIRE);
    }
    if (id == "mon_sanctum_wyrm") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_SANCTUM_WYRM_NAME", 96, 17, 360, 3, 8, 6, 5, 5,
            std::vector<D>{D::ICE, D::HOLY}, std::vector<D>{D::FIRE, D::ARCANE}, false, D::FIRE);
    }
    if (id == "mon_elder_wyrm") {
        return std::make_shared<ConcreteMonster>(
            id, "MONSTER_MON_ELDER_WYRM_NAME", 240, 17, 1200, 3, 10, 8, 6, 6,
            std::vector<D>{D::ICE, D::HOLY, D::LIGHTNING}, std::vector<D>{D::FIRE, D::POISON}, true, D::FIRE);
    }

    return nullptr;
}

EncounterTier MonsterFactory::tierForFloor(int floorNumber) {
    switch (std::clamp(floorNumber, 1, 5)) {
        case 1: return EncounterTier::EARLY;
        case 2: return EncounterTier::MIDDLE;
        case 3: return EncounterTier::LATE;
        case 4: return EncounterTier::DEEP;
        default: return EncounterTier::SANCTUM;
    }
}

std::shared_ptr<Monster> MonsterFactory::createRandomMonster() {
    return createRandomMonster(EncounterTier::MIDDLE, SessionRng::global());
}

std::vector<std::string> MonsterFactory::getEncounterPool(EncounterTier tier) {
    switch (tier) {
        case EncounterTier::EARLY:
            return {"mon_kobold", "mon_goblin", "mon_skeleton", "mon_crypt_rat"};
        case EncounterTier::MIDDLE:
            return {"mon_giant_spider", "mon_goblin_shaman", "mon_drowned_knight", "mon_mire_slime"};
        case EncounterTier::LATE:
            return {"mon_ghoul", "mon_orc", "mon_bone_archer", "mon_wraith"};
        case EncounterTier::DEEP:
            return {"mon_orc_berserker", "mon_ember_golem", "mon_ash_cultist", "mon_hellhound"};
        case EncounterTier::SANCTUM:
            return {"mon_drake_guard", "mon_dragon_whelp", "mon_sanctum_wyrm"};
    }
    return {"mon_kobold"};
}

std::shared_ptr<Monster> MonsterFactory::createRandomMonster(EncounterTier tier, SessionRng& random) {
    const auto pool = getEncounterPool(tier);
    if (pool.empty()) return nullptr;
    const int idx = random.rollRange(0, static_cast<int>(pool.size()) - 1);
    return createMonster(pool[static_cast<std::size_t>(idx)]);
}

std::vector<std::string> MonsterFactory::getRegisteredIds() {
    return {
        "mon_kobold", "mon_goblin", "mon_skeleton", "mon_crypt_rat", "mon_crypt_warden",
        "mon_giant_spider", "mon_goblin_shaman", "mon_drowned_knight", "mon_mire_slime", "mon_bog_hydra",
        "mon_ghoul", "mon_orc", "mon_bone_archer", "mon_wraith", "mon_ossuary_lich",
        "mon_orc_berserker", "mon_ember_golem", "mon_ash_cultist", "mon_hellhound", "mon_iron_colossus",
        "mon_drake_guard", "mon_dragon_whelp", "mon_sanctum_wyrm", "mon_elder_wyrm"
    };
}

std::vector<std::string> MonsterFactory::getDropItemIds(const std::string& monsterId) {
    if (monsterId == "mon_kobold") return {"pot_heal", "wpn_dagger"};
    if (monsterId == "mon_goblin") return {"pot_mana", "bomb_fire"};
    if (monsterId == "mon_skeleton") return {"wpn_greatsword", "shd_kite"};
    if (monsterId == "mon_crypt_rat") return {"scr_cure"};
    if (monsterId == "mon_crypt_warden") return {"wpn_halberd", "acc_iron_ring"};
    if (monsterId == "mon_giant_spider") return {"scr_cure", "pot_dexterity"};
    if (monsterId == "mon_goblin_shaman") return {"pot_mana", "wpn_frost_wand"};
    if (monsterId == "mon_drowned_knight") return {"arm_chain", "shd_kite"};
    if (monsterId == "mon_mire_slime") return {"bomb_frost", "pot_greater_heal"};
    if (monsterId == "mon_bog_hydra") return {"acc_shadow_cloak", "pot_panacea"};
    if (monsterId == "mon_ghoul") return {"pot_greater_heal", "pot_strength"};
    if (monsterId == "mon_orc") return {"wpn_rapier", "arm_plate"};
    if (monsterId == "mon_bone_archer") return {"wpn_longbow", "pot_dexterity"};
    if (monsterId == "mon_wraith") return {"acc_arcane_signet", "pot_greater_mana"};
    if (monsterId == "mon_ossuary_lich") return {"arm_archmage_robe", "wpn_sunmace"};
    if (monsterId == "mon_orc_berserker") return {"wpn_obsidian_axe", "pot_strength"};
    if (monsterId == "mon_ember_golem") return {"arm_obsidian_plate", "bomb_fire"};
    if (monsterId == "mon_ash_cultist") return {"wpn_flame_scepter", "pot_greater_mana"};
    if (monsterId == "mon_hellhound") return {"pot_elixir", "bomb_fire"};
    if (monsterId == "mon_iron_colossus") return {"shd_aegis", "arm_obsidian_plate"};
    if (monsterId == "mon_drake_guard") return {"wpn_stormbow", "acc_sun_amulet"};
    if (monsterId == "mon_dragon_whelp") return {"arm_plate", "shd_tower"};
    if (monsterId == "mon_sanctum_wyrm") return {"wpn_archmage_staff", "acc_dragon_heart"};
    if (monsterId == "mon_elder_wyrm") return {"wpn_dragonbane", "arm_dragonscale"};
    return {};
}

int MonsterFactory::getMonsterFloor(const std::string& monsterId) {
    if (monsterId == "mon_kobold" || monsterId == "mon_goblin" || monsterId == "mon_skeleton" ||
        monsterId == "mon_crypt_rat" || monsterId == "mon_crypt_warden") return 1;
    if (monsterId == "mon_giant_spider" || monsterId == "mon_goblin_shaman" ||
        monsterId == "mon_drowned_knight" || monsterId == "mon_mire_slime" ||
        monsterId == "mon_bog_hydra") return 2;
    if (monsterId == "mon_ghoul" || monsterId == "mon_orc" || monsterId == "mon_bone_archer" ||
        monsterId == "mon_wraith" || monsterId == "mon_ossuary_lich") return 3;
    if (monsterId == "mon_orc_berserker" || monsterId == "mon_ember_golem" ||
        monsterId == "mon_ash_cultist" || monsterId == "mon_hellhound" ||
        monsterId == "mon_iron_colossus") return 4;
    return 5;
}

} // namespace crawl
