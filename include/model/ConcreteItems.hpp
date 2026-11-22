#ifndef CONCRETE_ITEMS_HPP
#define CONCRETE_ITEMS_HPP

#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/Character.hpp"
#include "model/Equipment.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace crawl {

class ConsumableItem : public Item {
public:
    ConsumableItem(std::string id, int goldValue, ItemRarity rarity = ItemRarity::COMMON)
        : m_id(std::move(id)), m_goldValue(goldValue), m_rarity(rarity) {}
    ~ConsumableItem() override = default;

    [[nodiscard]] std::string getId() const override { return m_id; }
    [[nodiscard]] std::string getName() const override {
        return LocalizationManager::getInstance().getContent("ITEM", m_id, "NAME");
    }
    [[nodiscard]] int getGoldValue() const override { return m_goldValue; }
    [[nodiscard]] std::string getDescription() const override {
        return LocalizationManager::getInstance().getContent("ITEM", m_id, "DESC");
    }
    [[nodiscard]] bool isEquipment() const override { return false; }
    [[nodiscard]] ItemRarity getRarity() const override { return m_rarity; }
    [[nodiscard]] virtual bool isOffensiveBomb() const { return false; }
    [[nodiscard]] virtual DamageType getBombDamageType() const { return DamageType::FIRE; }
    [[nodiscard]] virtual int getBombPower() const { return 0; }

    virtual void applyEffect(Character& target,
                             std::vector<std::shared_ptr<Character>>& party,
                             std::vector<std::string>& logOutput) = 0;

protected:
    std::string m_id;
    int m_goldValue;
    ItemRarity m_rarity;
};

class HealPotionItem : public ConsumableItem {
public:
    HealPotionItem() : ConsumableItem("pot_heal", 18, ItemRarity::COMMON) {}
    [[nodiscard]] std::shared_ptr<Item> clone() const override { return std::make_shared<HealPotionItem>(*this); }
    void applyEffect(Character& target, std::vector<std::shared_ptr<Character>>& /*party*/,
                     std::vector<std::string>& logOutput) override {
        const int amt = SessionRng::global().rollDie(6) + SessionRng::global().rollDie(6) + 4;
        target.heal(amt);
        logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_HEAL", {
            {"target", target.getName()}, {"item", getName()}, {"amount", std::to_string(amt)}}));
    }
};

class GreaterHealPotionItem : public ConsumableItem {
public:
    GreaterHealPotionItem() : ConsumableItem("pot_greater_heal", 48, ItemRarity::UNCOMMON) {}
    [[nodiscard]] std::shared_ptr<Item> clone() const override { return std::make_shared<GreaterHealPotionItem>(*this); }
    void applyEffect(Character& target, std::vector<std::shared_ptr<Character>>& /*party*/,
                     std::vector<std::string>& logOutput) override {
        int amt = 10;
        for (int i = 0; i < 4; ++i) amt += SessionRng::global().rollDie(6);
        target.heal(amt);
        logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_HEAL", {
            {"target", target.getName()}, {"item", getName()}, {"amount", std::to_string(amt)}}));
    }
};

class ElixirItem : public ConsumableItem {
public:
    ElixirItem() : ConsumableItem("pot_elixir", 120, ItemRarity::EPIC) {}
    [[nodiscard]] std::shared_ptr<Item> clone() const override { return std::make_shared<ElixirItem>(*this); }
    void applyEffect(Character& target, std::vector<std::shared_ptr<Character>>& /*party*/,
                     std::vector<std::string>& logOutput) override {
        if (target.isDead()) {
            target.revive(std::max(1, target.getMaxHp() / 2));
        } else {
            target.heal(target.getMaxHp());
        }
        target.cureAllDebuffs();
        target.recoverSpellSlot(2);
        logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_ELIXIR", {
            {"target", target.getName()}, {"item", getName()}}));
    }
};

class ManaPotionItem : public ConsumableItem {
public:
    ManaPotionItem() : ConsumableItem("pot_mana", 32, ItemRarity::COMMON) {}
    [[nodiscard]] std::shared_ptr<Item> clone() const override { return std::make_shared<ManaPotionItem>(*this); }
    void applyEffect(Character& target, std::vector<std::shared_ptr<Character>>& /*party*/,
                     std::vector<std::string>& logOutput) override {
        if (target.getMaxSpellSlots() <= 0) {
            logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_INVALID_CASTER", {
                {"target", target.getName()}, {"item", getName()}}));
            return;
        }
        const int prev = target.getSpellSlots();
        target.recoverSpellSlot(1);
        if (target.getSpellSlots() > prev) {
            logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_SLOT_RECOVERED", {
                {"target", target.getName()}, {"item", getName()}}));
        } else {
            logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_SLOT_FULL", {
                {"target", target.getName()}}));
        }
    }
};

class GreaterManaPotionItem : public ConsumableItem {
public:
    GreaterManaPotionItem() : ConsumableItem("pot_greater_mana", 68, ItemRarity::RARE) {}
    [[nodiscard]] std::shared_ptr<Item> clone() const override { return std::make_shared<GreaterManaPotionItem>(*this); }
    void applyEffect(Character& target, std::vector<std::shared_ptr<Character>>& /*party*/,
                     std::vector<std::string>& logOutput) override {
        if (target.getMaxSpellSlots() <= 0) {
            logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_INVALID_CASTER", {
                {"target", target.getName()}, {"item", getName()}}));
            return;
        }
        target.recoverSpellSlot(3);
        logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_SLOT_RECOVERED", {
            {"target", target.getName()}, {"item", getName()}}));
    }
};

class StrengthPotionItem : public ConsumableItem {
public:
    StrengthPotionItem() : ConsumableItem("pot_strength", 28, ItemRarity::UNCOMMON) {}
    [[nodiscard]] std::shared_ptr<Item> clone() const override { return std::make_shared<StrengthPotionItem>(*this); }
    void applyEffect(Character& target, std::vector<std::shared_ptr<Character>>& /*party*/,
                     std::vector<std::string>& logOutput) override {
        target.applyStrBuff(4, 5);
        logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_BUFF", {
            {"target", target.getName()}, {"item", getName()}}));
    }
};

class DexterityPotionItem : public ConsumableItem {
public:
    DexterityPotionItem() : ConsumableItem("pot_dexterity", 28, ItemRarity::UNCOMMON) {}
    [[nodiscard]] std::shared_ptr<Item> clone() const override { return std::make_shared<DexterityPotionItem>(*this); }
    void applyEffect(Character& target, std::vector<std::shared_ptr<Character>>& /*party*/,
                     std::vector<std::string>& logOutput) override {
        target.applyDexBuff(4, 5);
        logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_BUFF", {
            {"target", target.getName()}, {"item", getName()}}));
    }
};

class CureScrollItem : public ConsumableItem {
public:
    CureScrollItem() : ConsumableItem("scr_cure", 22, ItemRarity::COMMON) {}
    [[nodiscard]] std::shared_ptr<Item> clone() const override { return std::make_shared<CureScrollItem>(*this); }
    void applyEffect(Character& target, std::vector<std::shared_ptr<Character>>& /*party*/,
                     std::vector<std::string>& logOutput) override {
        if (target.hasAnyDebuff()) {
            target.cureAllDebuffs();
            logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_CURED", {
                {"target", target.getName()}, {"item", getName()}}));
        } else {
            logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_NOT_POISONED", {
                {"target", target.getName()}}));
        }
    }
};

class PanaceaItem : public ConsumableItem {
public:
    PanaceaItem() : ConsumableItem("pot_panacea", 55, ItemRarity::RARE) {}
    [[nodiscard]] std::shared_ptr<Item> clone() const override { return std::make_shared<PanaceaItem>(*this); }
    void applyEffect(Character& /*target*/, std::vector<std::shared_ptr<Character>>& party,
                     std::vector<std::string>& logOutput) override {
        for (auto& m : party) {
            if (m && !m->isDead()) {
                m->cureAllDebuffs();
                m->heal(8);
            }
        }
        logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_PANACEA", {
            {"item", getName()}}));
    }
};

class BombItem : public ConsumableItem {
public:
    BombItem(std::string id, int goldValue, DamageType elem, int power, ItemRarity rarity)
        : ConsumableItem(std::move(id), goldValue, rarity), m_elem(elem), m_power(power) {}
    [[nodiscard]] std::shared_ptr<Item> clone() const override { return std::make_shared<BombItem>(*this); }
    [[nodiscard]] bool isOffensiveBomb() const override { return true; }
    [[nodiscard]] DamageType getBombDamageType() const override { return m_elem; }
    [[nodiscard]] int getBombPower() const override { return m_power; }
    void applyEffect(Character& /*target*/, std::vector<std::shared_ptr<Character>>& /*party*/,
                     std::vector<std::string>& /*logOutput*/) override {}

private:
    DamageType m_elem;
    int m_power;
};

// 공통 장비 구현 베이스 (희귀도·강화 +0~+5·접사·JSON 직렬화)
class ConcreteEquipment : public Equipment {
public:
    ConcreteEquipment(std::string id, int goldValue, EquipSlot slot, int acBonus,
                      int diceCount, int diceSides, DamageType dmgType, WeaponReach reach,
                      std::string bonusStat, int bonusVal, ItemRarity rarity = ItemRarity::COMMON)
        : m_id(std::move(id)),
          m_goldValue(goldValue),
          m_slot(slot),
          m_acBonus(acBonus),
          m_diceCount(diceCount),
          m_diceSides(diceSides),
          m_damageType(dmgType),
          m_reach(reach),
          m_bonusStat(std::move(bonusStat)),
          m_bonusVal(bonusVal),
          m_rarity(rarity) {}

    [[nodiscard]] std::string getId() const override { return m_id; }
    [[nodiscard]] std::string getName() const override {
        return LocalizationManager::getInstance().getContent("ITEM", m_id, "NAME");
    }
    [[nodiscard]] std::string getDisplayName() const override {
        std::string name = getName();
        if (!m_affix.affixKey.empty()) {
            name = LocalizationManager::getInstance().get(m_affix.affixKey) + " " + name;
        }
        if (m_upgradeLevel > 0) {
            name += " +" + std::to_string(m_upgradeLevel);
        }
        return name;
    }
    [[nodiscard]] int getGoldValue() const override {
        const float rarityMul = 1.0f + static_cast<int>(m_rarity) * 0.45f;
        const float upgMul = 1.0f + m_upgradeLevel * 0.25f;
        return std::max(1, static_cast<int>(m_goldValue * rarityMul * upgMul));
    }
    [[nodiscard]] std::string getDescription() const override {
        return LocalizationManager::getInstance().getContent("ITEM", m_id, "DESC");
    }

    [[nodiscard]] ItemRarity getRarity() const override { return m_rarity; }
    [[nodiscard]] EquipSlot getSlot() const override { return m_slot; }
    [[nodiscard]] int getAcBonus() const override {
        int ac = m_acBonus + m_affix.bonusAc;
        if (m_slot == EquipSlot::ARMOR || m_slot == EquipSlot::SHIELD) {
            ac += (m_upgradeLevel + 1) / 2;
        }
        return ac;
    }
    [[nodiscard]] int getDamageDiceCount() const override { return m_diceCount; }
    [[nodiscard]] int getDamageDiceSides() const override { return m_diceSides; }
    [[nodiscard]] DamageType getDamageType() const override {
        if (m_affix.bonusElementDamage > 0 && m_slot == EquipSlot::WEAPON) {
            return m_affix.bonusElement;
        }
        return m_damageType;
    }
    [[nodiscard]] WeaponReach getReach() const override { return m_reach; }
    [[nodiscard]] int getUpgradeLevel() const override { return m_upgradeLevel; }
    [[nodiscard]] const ItemAffix& getAffix() const override { return m_affix; }
    [[nodiscard]] int getUpgradeDamageBonus() const override {
        if (m_slot != EquipSlot::WEAPON) return 0;
        return m_upgradeLevel * 2 + m_affix.bonusElementDamage + static_cast<int>(m_rarity);
    }
    [[nodiscard]] int getBonusHp() const override {
        int hp = m_affix.bonusHp;
        if (m_slot == EquipSlot::ARMOR || m_slot == EquipSlot::SHIELD) {
            hp += m_upgradeLevel * 3;
        }
        if (m_slot == EquipSlot::ACCESSORY) {
            hp += m_upgradeLevel * 4;
        }
        return hp;
    }

    [[nodiscard]] int getStatBonus(const std::string& statName) const override {
        int val = (statName == m_bonusStat) ? m_bonusVal : 0;
        if (m_slot == EquipSlot::ACCESSORY && statName == m_bonusStat && !m_bonusStat.empty()) {
            val += m_upgradeLevel / 2;
        }
        if (statName == "str") val += m_affix.bonusStr;
        else if (statName == "dex") val += m_affix.bonusDex;
        else if (statName == "con") val += m_affix.bonusCon;
        else if (statName == "int") val += m_affix.bonusInt;
        else if (statName == "wis") val += m_affix.bonusWis;
        return val;
    }

    void setRarity(ItemRarity rarity) override { m_rarity = rarity; }
    void setUpgradeLevel(int level) override { m_upgradeLevel = std::clamp(level, 0, MAX_UPGRADE_LEVEL); }
    void setAffix(const ItemAffix& affix) override { m_affix = affix; }

    [[nodiscard]] std::shared_ptr<Item> clone() const override {
        return std::make_shared<ConcreteEquipment>(*this);
    }

    [[nodiscard]] nlohmann::json toJson() const override {
        nlohmann::json j = {
            {"id", m_id},
            {"rarity", static_cast<int>(m_rarity)},
            {"upgradeLevel", m_upgradeLevel}
        };
        if (!m_affix.empty()) {
            j["affix"] = m_affix.toJson();
        }
        return j;
    }

private:
    std::string m_id;
    int m_goldValue;
    EquipSlot m_slot;
    int m_acBonus;
    int m_diceCount;
    int m_diceSides;
    DamageType m_damageType;
    WeaponReach m_reach;
    std::string m_bonusStat;
    int m_bonusVal;
    ItemRarity m_rarity;
    int m_upgradeLevel = 0;
    ItemAffix m_affix;
};

} // namespace crawl

#endif // CONCRETE_ITEMS_HPP
