#include "model/ItemFactory.hpp"
#include "core/SessionRng.hpp"
#include "model/ConcreteItems.hpp"

#include <algorithm>
#include <stdexcept>

namespace crawl {

std::shared_ptr<Item> ItemFactory::createItem(const std::string& id) {
    // === 무기 (14종: 근접/장병기/원거리 + 속성 무기) ===
    if (id == "wpn_dagger") {
        return std::make_shared<ConcreteEquipment>(id, 12, EquipSlot::WEAPON, 0, 1, 4,
            DamageType::PIERCING, WeaponReach::MELEE, "dex", 1, ItemRarity::COMMON);
    }
    if (id == "wpn_longsword") {
        return std::make_shared<ConcreteEquipment>(id, 30, EquipSlot::WEAPON, 0, 1, 8,
            DamageType::SLASHING, WeaponReach::MELEE, "", 0, ItemRarity::COMMON);
    }
    if (id == "wpn_mace") {
        return std::make_shared<ConcreteEquipment>(id, 22, EquipSlot::WEAPON, 0, 1, 6,
            DamageType::BLUDGEONING, WeaponReach::MELEE, "", 0, ItemRarity::COMMON);
    }
    if (id == "wpn_staff") {
        return std::make_shared<ConcreteEquipment>(id, 18, EquipSlot::WEAPON, 0, 1, 4,
            DamageType::ARCANE, WeaponReach::REACH, "int", 1, ItemRarity::COMMON);
    }
    if (id == "wpn_rapier") {
        return std::make_shared<ConcreteEquipment>(id, 46, EquipSlot::WEAPON, 0, 1, 8,
            DamageType::PIERCING, WeaponReach::MELEE, "dex", 1, ItemRarity::UNCOMMON);
    }
    if (id == "wpn_greatsword") {
        return std::make_shared<ConcreteEquipment>(id, 72, EquipSlot::WEAPON, 0, 2, 6,
            DamageType::SLASHING, WeaponReach::MELEE, "str", 1, ItemRarity::UNCOMMON);
    }
    if (id == "wpn_halberd") {
        return std::make_shared<ConcreteEquipment>(id, 85, EquipSlot::WEAPON, 0, 1, 10,
            DamageType::PIERCING, WeaponReach::REACH, "str", 1, ItemRarity::UNCOMMON);
    }
    if (id == "wpn_longbow") {
        return std::make_shared<ConcreteEquipment>(id, 78, EquipSlot::WEAPON, 0, 1, 8,
            DamageType::PIERCING, WeaponReach::RANGED, "dex", 2, ItemRarity::UNCOMMON);
    }
    if (id == "wpn_frost_wand") {
        return std::make_shared<ConcreteEquipment>(id, 95, EquipSlot::WEAPON, 0, 1, 6,
            DamageType::ICE, WeaponReach::RANGED, "int", 2, ItemRarity::RARE);
    }
    if (id == "wpn_sunmace") {
        return std::make_shared<ConcreteEquipment>(id, 110, EquipSlot::WEAPON, 0, 1, 8,
            DamageType::HOLY, WeaponReach::MELEE, "wis", 2, ItemRarity::RARE);
    }
    if (id == "wpn_flame_scepter") {
        return std::make_shared<ConcreteEquipment>(id, 135, EquipSlot::WEAPON, 0, 2, 5,
            DamageType::FIRE, WeaponReach::RANGED, "int", 3, ItemRarity::RARE);
    }
    if (id == "wpn_stormbow") {
        return std::make_shared<ConcreteEquipment>(id, 165, EquipSlot::WEAPON, 0, 2, 6,
            DamageType::LIGHTNING, WeaponReach::RANGED, "dex", 3, ItemRarity::EPIC);
    }
    if (id == "wpn_obsidian_axe") {
        return std::make_shared<ConcreteEquipment>(id, 195, EquipSlot::WEAPON, 0, 2, 8,
            DamageType::FIRE, WeaponReach::MELEE, "str", 3, ItemRarity::EPIC);
    }
    if (id == "wpn_dragonbane") {
        return std::make_shared<ConcreteEquipment>(id, 320, EquipSlot::WEAPON, 1, 3, 6,
            DamageType::HOLY, WeaponReach::REACH, "str", 4, ItemRarity::LEGENDARY);
    }
    if (id == "wpn_archmage_staff") {
        return std::make_shared<ConcreteEquipment>(id, 290, EquipSlot::WEAPON, 0, 2, 6,
            DamageType::ARCANE, WeaponReach::RANGED, "int", 4, ItemRarity::LEGENDARY);
    }

    // === 갑옷 (8종) ===
    if (id == "arm_robe") {
        return std::make_shared<ConcreteEquipment>(id, 10, EquipSlot::ARMOR, 1, 0, 0,
            DamageType::BLUDGEONING, WeaponReach::MELEE, "int", 1, ItemRarity::COMMON);
    }
    if (id == "arm_leather") {
        return std::make_shared<ConcreteEquipment>(id, 22, EquipSlot::ARMOR, 2, 0, 0,
            DamageType::BLUDGEONING, WeaponReach::MELEE, "dex", 1, ItemRarity::COMMON);
    }
    if (id == "arm_scale") {
        return std::make_shared<ConcreteEquipment>(id, 48, EquipSlot::ARMOR, 4, 0, 0,
            DamageType::BLUDGEONING, WeaponReach::MELEE, "", 0, ItemRarity::COMMON);
    }
    if (id == "arm_chain") {
        return std::make_shared<ConcreteEquipment>(id, 82, EquipSlot::ARMOR, 6, 0, 0,
            DamageType::BLUDGEONING, WeaponReach::MELEE, "con", 1, ItemRarity::UNCOMMON);
    }
    if (id == "arm_plate") {
        return std::make_shared<ConcreteEquipment>(id, 135, EquipSlot::ARMOR, 8, 0, 0,
            DamageType::BLUDGEONING, WeaponReach::MELEE, "con", 2, ItemRarity::RARE);
    }
    if (id == "arm_archmage_robe") {
        return std::make_shared<ConcreteEquipment>(id, 155, EquipSlot::ARMOR, 4, 0, 0,
            DamageType::ARCANE, WeaponReach::MELEE, "int", 3, ItemRarity::EPIC);
    }
    if (id == "arm_obsidian_plate") {
        return std::make_shared<ConcreteEquipment>(id, 220, EquipSlot::ARMOR, 10, 0, 0,
            DamageType::FIRE, WeaponReach::MELEE, "con", 3, ItemRarity::EPIC);
    }
    if (id == "arm_dragonscale") {
        return std::make_shared<ConcreteEquipment>(id, 340, EquipSlot::ARMOR, 12, 0, 0,
            DamageType::FIRE, WeaponReach::MELEE, "con", 4, ItemRarity::LEGENDARY);
    }

    // === 방패 (4종) ===
    if (id == "shd_round") {
        return std::make_shared<ConcreteEquipment>(id, 24, EquipSlot::SHIELD, 2, 0, 0,
            DamageType::BLUDGEONING, WeaponReach::MELEE, "", 0, ItemRarity::COMMON);
    }
    if (id == "shd_kite") {
        return std::make_shared<ConcreteEquipment>(id, 55, EquipSlot::SHIELD, 3, 0, 0,
            DamageType::BLUDGEONING, WeaponReach::MELEE, "con", 1, ItemRarity::UNCOMMON);
    }
    if (id == "shd_tower") {
        return std::make_shared<ConcreteEquipment>(id, 95, EquipSlot::SHIELD, 4, 0, 0,
            DamageType::BLUDGEONING, WeaponReach::MELEE, "con", 2, ItemRarity::RARE);
    }
    if (id == "shd_aegis") {
        return std::make_shared<ConcreteEquipment>(id, 240, EquipSlot::SHIELD, 6, 0, 0,
            DamageType::HOLY, WeaponReach::MELEE, "wis", 3, ItemRarity::LEGENDARY);
    }

    // === 장신구 (5종) ===
    if (id == "acc_iron_ring") {
        return std::make_shared<ConcreteEquipment>(id, 45, EquipSlot::ACCESSORY, 1, 0, 0,
            DamageType::BLUDGEONING, WeaponReach::MELEE, "con", 1, ItemRarity::UNCOMMON);
    }
    if (id == "acc_arcane_signet") {
        return std::make_shared<ConcreteEquipment>(id, 90, EquipSlot::ACCESSORY, 0, 0, 0,
            DamageType::ARCANE, WeaponReach::MELEE, "int", 2, ItemRarity::RARE);
    }
    if (id == "acc_shadow_cloak") {
        return std::make_shared<ConcreteEquipment>(id, 110, EquipSlot::ACCESSORY, 2, 0, 0,
            DamageType::PIERCING, WeaponReach::MELEE, "dex", 2, ItemRarity::RARE);
    }
    if (id == "acc_sun_amulet") {
        return std::make_shared<ConcreteEquipment>(id, 150, EquipSlot::ACCESSORY, 2, 0, 0,
            DamageType::HOLY, WeaponReach::MELEE, "wis", 3, ItemRarity::EPIC);
    }
    if (id == "acc_dragon_heart") {
        return std::make_shared<ConcreteEquipment>(id, 280, EquipSlot::ACCESSORY, 3, 0, 0,
            DamageType::FIRE, WeaponReach::MELEE, "str", 3, ItemRarity::LEGENDARY);
    }

    // === 소모품 (11종) ===
    if (id == "pot_heal") return std::make_shared<HealPotionItem>();
    if (id == "pot_greater_heal") return std::make_shared<GreaterHealPotionItem>();
    if (id == "pot_elixir") return std::make_shared<ElixirItem>();
    if (id == "pot_mana") return std::make_shared<ManaPotionItem>();
    if (id == "pot_greater_mana") return std::make_shared<GreaterManaPotionItem>();
    if (id == "pot_strength") return std::make_shared<StrengthPotionItem>();
    if (id == "pot_dexterity") return std::make_shared<DexterityPotionItem>();
    if (id == "scr_cure") return std::make_shared<CureScrollItem>();
    if (id == "pot_panacea") return std::make_shared<PanaceaItem>();
    if (id == "bomb_fire") {
        return std::make_shared<BombItem>(id, 35, DamageType::FIRE, 14, ItemRarity::UNCOMMON);
    }
    if (id == "bomb_frost") {
        return std::make_shared<BombItem>(id, 35, DamageType::ICE, 14, ItemRarity::UNCOMMON);
    }

    return nullptr;
}

std::shared_ptr<Item> ItemFactory::createItemFromJson(const nlohmann::json& j) {
    if (j.is_string()) {
        return createItem(j.get<std::string>());
    }
    if (!j.is_object() || !j.contains("id")) {
        throw std::runtime_error("invalid item json");
    }
    auto item = createItem(j.at("id").get<std::string>());
    if (!item) throw std::runtime_error("unknown item id: " + j.at("id").get<std::string>());

    if (auto eq = std::dynamic_pointer_cast<Equipment>(item)) {
        if (j.contains("rarity")) {
            const int r = j["rarity"].get<int>();
            if (r < 0 || r > static_cast<int>(ItemRarity::LEGENDARY)) {
                throw std::runtime_error("invalid item rarity");
            }
            eq->setRarity(static_cast<ItemRarity>(r));
        }
        if (j.contains("upgradeLevel")) {
            eq->setUpgradeLevel(j["upgradeLevel"].get<int>());
        }
        if (j.contains("affix")) {
            eq->setAffix(ItemAffix::fromJson(j["affix"]));
        }
    }
    return item;
}

std::vector<std::shared_ptr<Item>> ItemFactory::getShopCatalog(int deepestFloor) {
    std::vector<std::shared_ptr<Item>> catalog = {
        createItem("pot_heal"),
        createItem("pot_mana"),
        createItem("scr_cure"),
        createItem("bomb_fire"),
        createItem("bomb_frost"),
        createItem("wpn_dagger"),
        createItem("wpn_longsword"),
        createItem("wpn_mace"),
        createItem("wpn_halberd"),
        createItem("wpn_longbow"),
        createItem("arm_leather"),
        createItem("arm_scale"),
        createItem("shd_round"),
        createItem("acc_iron_ring")
    };
    if (deepestFloor >= 2) {
        catalog.push_back(createItem("pot_greater_heal"));
        catalog.push_back(createItem("pot_strength"));
        catalog.push_back(createItem("pot_dexterity"));
        catalog.push_back(createItem("wpn_rapier"));
        catalog.push_back(createItem("wpn_frost_wand"));
        catalog.push_back(createItem("arm_chain"));
        catalog.push_back(createItem("shd_kite"));
    }
    if (deepestFloor >= 3) {
        catalog.push_back(createItem("pot_greater_mana"));
        catalog.push_back(createItem("pot_panacea"));
        catalog.push_back(createItem("wpn_sunmace"));
        catalog.push_back(createItem("arm_plate"));
        catalog.push_back(createItem("shd_tower"));
        catalog.push_back(createItem("acc_arcane_signet"));
    }
    if (deepestFloor >= 4) {
        catalog.push_back(createItem("pot_elixir"));
        catalog.push_back(createItem("wpn_flame_scepter"));
        catalog.push_back(createItem("wpn_stormbow"));
        catalog.push_back(createItem("arm_obsidian_plate"));
        catalog.push_back(createItem("acc_sun_amulet"));
    }
    return catalog;
}

std::vector<std::string> ItemFactory::getRegisteredIds() {
    return {
        "wpn_dagger", "wpn_longsword", "wpn_mace", "wpn_staff", "wpn_rapier",
        "wpn_greatsword", "wpn_halberd", "wpn_longbow", "wpn_frost_wand", "wpn_sunmace",
        "wpn_flame_scepter", "wpn_stormbow", "wpn_obsidian_axe", "wpn_dragonbane",
        "wpn_archmage_staff",
        "arm_robe", "arm_leather", "arm_scale", "arm_chain", "arm_plate",
        "arm_archmage_robe", "arm_obsidian_plate", "arm_dragonscale",
        "shd_round", "shd_kite", "shd_tower", "shd_aegis",
        "acc_iron_ring", "acc_arcane_signet", "acc_shadow_cloak", "acc_sun_amulet",
        "acc_dragon_heart",
        "pot_heal", "pot_greater_heal", "pot_elixir", "pot_mana", "pot_greater_mana",
        "pot_strength", "pot_dexterity", "scr_cure", "pot_panacea", "bomb_fire", "bomb_frost"
    };
}

ItemAffix ItemFactory::rollAffix(ItemRarity rarity, SessionRng& rng) {
    ItemAffix a;
    const int scale = std::max(1, static_cast<int>(rarity));
    const int roll = rng.rollRange(0, 6);
    switch (roll) {
        case 0:
            a.affixKey = "AFFIX_FIERCE";
            a.bonusStr = scale;
            a.critBonusPct = scale * 2;
            break;
        case 1:
            a.affixKey = "AFFIX_SWIFT";
            a.bonusDex = scale;
            a.critBonusPct = scale * 3;
            break;
        case 2:
            a.affixKey = "AFFIX_STALWART";
            a.bonusCon = scale;
            a.bonusAc = std::max(1, scale / 2);
            a.bonusHp = scale * 4;
            break;
        case 3:
            a.affixKey = "AFFIX_ARCANE";
            a.bonusInt = scale;
            a.bonusElement = DamageType::ARCANE;
            a.bonusElementDamage = scale;
            break;
        case 4:
            a.affixKey = "AFFIX_BLAZING";
            a.bonusElement = DamageType::FIRE;
            a.bonusElementDamage = scale + 1;
            break;
        case 5:
            a.affixKey = "AFFIX_FROZEN";
            a.bonusElement = DamageType::ICE;
            a.bonusElementDamage = scale + 1;
            break;
        default:
            a.affixKey = "AFFIX_VAMPIRIC";
            a.lifestealPct = std::min(20, 4 + scale * 3);
            a.bonusWis = std::max(1, scale / 2);
            break;
    }
    return a;
}

std::shared_ptr<Item> ItemFactory::rollChestLoot(int floorNumber, SessionRng& rng) {
    static const std::vector<std::vector<std::string>> pools = {
        {"wpn_rapier", "wpn_halberd", "arm_chain", "shd_kite", "acc_iron_ring", "pot_greater_heal"},
        {"wpn_greatsword", "wpn_longbow", "wpn_frost_wand", "arm_chain", "acc_shadow_cloak", "pot_greater_mana"},
        {"wpn_sunmace", "wpn_flame_scepter", "arm_plate", "shd_tower", "acc_arcane_signet", "pot_panacea"},
        {"wpn_stormbow", "wpn_obsidian_axe", "arm_archmage_robe", "arm_obsidian_plate", "acc_sun_amulet", "pot_elixir"},
        {"wpn_dragonbane", "wpn_archmage_staff", "arm_dragonscale", "shd_aegis", "acc_dragon_heart", "pot_elixir"}
    };
    const int idx = std::clamp(floorNumber - 1, 0, static_cast<int>(pools.size()) - 1);
    const auto& pool = pools[static_cast<std::size_t>(idx)];
    auto item = createItem(pool[static_cast<std::size_t>(rng.rollRange(0, static_cast<int>(pool.size()) - 1))]);
    if (auto eq = std::dynamic_pointer_cast<Equipment>(item)) {
        const int rRoll = rng.rollRange(1, 100);
        ItemRarity rar = eq->getRarity();
        if (floorNumber >= 4 && rRoll > 65) rar = std::max(rar, ItemRarity::EPIC);
        else if (floorNumber >= 2 && rRoll > 50) rar = std::max(rar, ItemRarity::RARE);
        else if (rRoll > 35) rar = std::max(rar, ItemRarity::UNCOMMON);
        eq->setRarity(rar);
        if (rar >= ItemRarity::UNCOMMON) {
            eq->setAffix(rollAffix(rar, rng));
        }
        if (floorNumber >= 3 && rng.rollRange(1, 100) > 60) {
            eq->setUpgradeLevel(rng.rollRange(1, std::min(3, floorNumber - 1)));
        }
    }
    return item;
}

} // namespace crawl
