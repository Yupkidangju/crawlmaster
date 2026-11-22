#ifndef EQUIPMENT_HPP
#define EQUIPMENT_HPP

#include "model/Item.hpp"
#include <string>

namespace crawl {

enum class DamageType {
    SLASHING = 0,
    PIERCING = 1,
    BLUDGEONING = 2,
    FIRE = 3,
    ICE = 4,
    LIGHTNING = 5,
    HOLY = 6,
    POISON = 7,
    ARCANE = 8
};

enum class EquipSlot {
    WEAPON = 0,
    ARMOR = 1,
    SHIELD = 2,
    ACCESSORY = 3
};

enum class WeaponReach {
    MELEE = 0,
    REACH = 1,
    RANGED = 2
};

struct ItemAffix {
    std::string affixKey; // 예: "AFFIX_FIERCE", "AFFIX_STALWART"
    int bonusStr = 0;
    int bonusDex = 0;
    int bonusCon = 0;
    int bonusInt = 0;
    int bonusWis = 0;
    int bonusAc = 0;
    int bonusHp = 0;
    int critBonusPct = 0;
    int lifestealPct = 0;
    int bonusElementDamage = 0;
    DamageType bonusElement = DamageType::SLASHING;

    [[nodiscard]] bool empty() const {
        return affixKey.empty() && bonusStr == 0 && bonusDex == 0 && bonusCon == 0 &&
               bonusInt == 0 && bonusWis == 0 && bonusAc == 0 && bonusHp == 0 &&
               critBonusPct == 0 && lifestealPct == 0 && bonusElementDamage == 0;
    }

    [[nodiscard]] nlohmann::json toJson() const {
        return nlohmann::json{
            {"affixKey", affixKey},
            {"bonusStr", bonusStr},
            {"bonusDex", bonusDex},
            {"bonusCon", bonusCon},
            {"bonusInt", bonusInt},
            {"bonusWis", bonusWis},
            {"bonusAc", bonusAc},
            {"bonusHp", bonusHp},
            {"critBonusPct", critBonusPct},
            {"lifestealPct", lifestealPct},
            {"bonusElementDamage", bonusElementDamage},
            {"bonusElement", static_cast<int>(bonusElement)}
        };
    }

    static ItemAffix fromJson(const nlohmann::json& j) {
        ItemAffix a;
        if (!j.is_object()) return a;
        a.affixKey = j.value("affixKey", "");
        a.bonusStr = j.value("bonusStr", 0);
        a.bonusDex = j.value("bonusDex", 0);
        a.bonusCon = j.value("bonusCon", 0);
        a.bonusInt = j.value("bonusInt", 0);
        a.bonusWis = j.value("bonusWis", 0);
        a.bonusAc = j.value("bonusAc", 0);
        a.bonusHp = j.value("bonusHp", 0);
        a.critBonusPct = j.value("critBonusPct", 0);
        a.lifestealPct = j.value("lifestealPct", 0);
        a.bonusElementDamage = j.value("bonusElementDamage", 0);
        const int elem = j.value("bonusElement", 0);
        if (elem >= 0 && elem <= static_cast<int>(DamageType::ARCANE)) {
            a.bonusElement = static_cast<DamageType>(elem);
        }
        return a;
    }
};

class Equipment : public Item {
public:
    static constexpr int MAX_UPGRADE_LEVEL = 5;

    ~Equipment() override = default;

    [[nodiscard]] bool isEquipment() const override { return true; }

    [[nodiscard]] virtual EquipSlot getSlot() const = 0;
    [[nodiscard]] virtual int getAcBonus() const = 0;
    [[nodiscard]] virtual int getDamageDiceCount() const = 0;
    [[nodiscard]] virtual int getDamageDiceSides() const = 0;
    [[nodiscard]] virtual DamageType getDamageType() const = 0;
    [[nodiscard]] virtual int getStatBonus(const std::string& statName) const = 0;
    [[nodiscard]] virtual WeaponReach getReach() const { return WeaponReach::MELEE; }
    [[nodiscard]] virtual int getUpgradeLevel() const = 0;
    [[nodiscard]] virtual const ItemAffix& getAffix() const = 0;
    [[nodiscard]] virtual int getUpgradeDamageBonus() const { return 0; }
    [[nodiscard]] virtual int getBonusHp() const { return 0; }
    [[nodiscard]] virtual int getCritBonusPct() const { return getAffix().critBonusPct; }
    [[nodiscard]] virtual int getLifestealPct() const { return getAffix().lifestealPct; }
    [[nodiscard]] virtual std::string getDisplayName() const = 0;

    virtual void setRarity(ItemRarity rarity) = 0;
    virtual void setUpgradeLevel(int level) = 0;
    virtual void setAffix(const ItemAffix& affix) = 0;
};

} // namespace crawl

#endif // EQUIPMENT_HPP
