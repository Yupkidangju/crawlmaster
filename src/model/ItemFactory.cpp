// [v0.3.0] ItemFactory.cpp 신규 작성
// [v0.8.0] 신규 장비 5종(그레이트소드, 지팡이, 레이피어, 판금갑옷, 타워실드) 및 소모성 5종(고급치유, 마나, 힘, 민첩, 해독) 등록 및 상점 카탈로그 확장.
// spec.md의 아이템 기준표에 명시된 모든 아이템 데이터를 팩토리 코어에 매핑하여 동적 생성을 구현한다.

#include "model/ItemFactory.hpp"
#include "model/ConcreteItems.hpp"

namespace crawl {

std::shared_ptr<Item> ItemFactory::createItem(const std::string& id) {
    // === 무기류 ===
    if (id == "wpn_dagger") {
        return std::make_shared<WeaponItem>(
            "wpn_dagger", 10, 1, 4, "", 0, DamageType::PIERCING
        );
    } else if (id == "wpn_longsword") {
        return std::make_shared<WeaponItem>(
            "wpn_longsword", 30, 1, 8
        );
    } else if (id == "wpn_mace") {
        return std::make_shared<WeaponItem>(
            "wpn_mace", 20, 1, 6, "", 0, DamageType::BLUDGEONING
        );
    } else if (id == "wpn_greatsword") {
        return std::make_shared<WeaponItem>(
            "wpn_greatsword", 60, 2, 6
        );
    } else if (id == "wpn_staff") {
        // INT 능력치 +1 보너스 주입
        return std::make_shared<WeaponItem>(
            "wpn_staff", 15, 1, 4, "int", 1, DamageType::BLUDGEONING
        );
    } else if (id == "wpn_rapier") {
        return std::make_shared<WeaponItem>(
            "wpn_rapier", 40, 1, 8, "", 0, DamageType::PIERCING
        );
    } 
    // === 방어구류 ===
    else if (id == "arm_robe") {
        return std::make_shared<ArmorItem>(
            "arm_robe", 5, EquipSlot::ARMOR, 0
        );
    } else if (id == "arm_leather") {
        return std::make_shared<ArmorItem>(
            "arm_leather", 15, EquipSlot::ARMOR, 1
        );
    } else if (id == "arm_scale") {
        return std::make_shared<ArmorItem>(
            "arm_scale", 45, EquipSlot::ARMOR, 4
        );
    } else if (id == "arm_chain") {
        return std::make_shared<ArmorItem>(
            "arm_chain", 75, EquipSlot::ARMOR, 6
        );
    } else if (id == "arm_plate") {
        return std::make_shared<ArmorItem>(
            "arm_plate", 120, EquipSlot::ARMOR, 8
        );
    } 
    // === 방패류 ===
    else if (id == "shd_round") {
        return std::make_shared<ArmorItem>(
            "shd_round", 20, EquipSlot::SHIELD, 2
        );
    } else if (id == "shd_tower") {
        return std::make_shared<ArmorItem>(
            "shd_tower", 50, EquipSlot::SHIELD, 4
        );
    } 
    // === 소모품류 ===
    else if (id == "pot_heal") {
        return std::make_shared<HealPotionItem>();
    } else if (id == "pot_greater_heal") {
        return std::make_shared<GreaterHealPotionItem>();
    } else if (id == "pot_mana") {
        return std::make_shared<ManaPotionItem>();
    } else if (id == "pot_strength") {
        return std::make_shared<StrengthPotionItem>();
    } else if (id == "pot_dexterity") {
        return std::make_shared<DexterityPotionItem>();
    } else if (id == "scr_cure") {
        return std::make_shared<CureScrollItem>();
    }

    return nullptr;
}

std::vector<std::shared_ptr<Item>> ItemFactory::getShopCatalog() {
    // 상점에서 기본 판매 물품으로 등록할 아이템 카탈로그 리스트 반환
    return {
        createItem("wpn_dagger"),
        createItem("wpn_longsword"),
        createItem("wpn_mace"),
        createItem("wpn_greatsword"),
        createItem("wpn_staff"),
        createItem("wpn_rapier"),
        createItem("arm_leather"),
        createItem("arm_scale"),
        createItem("arm_chain"),
        createItem("arm_plate"),
        createItem("shd_round"),
        createItem("shd_tower"),
        createItem("pot_heal"),
        createItem("pot_greater_heal"),
        createItem("pot_mana"),
        createItem("pot_strength"),
        createItem("pot_dexterity"),
        createItem("scr_cure")
    };
}

std::vector<std::string> ItemFactory::getRegisteredIds() {
    return {
        "wpn_dagger", "wpn_longsword", "wpn_mace", "wpn_greatsword", "wpn_staff", "wpn_rapier",
        "arm_robe", "arm_leather", "arm_scale", "arm_chain", "arm_plate",
        "shd_round", "shd_tower", "pot_heal", "pot_greater_heal", "pot_mana",
        "pot_strength", "pot_dexterity", "scr_cure"
    };
}

} // namespace crawl
