// [v0.3.0] ConcreteItems.hpp 신규 작성
// [v0.8.0] 소모성 아이템에 applyEffect 가상 함수 추가 및 구체 물약/스크롤 6종 파생 클래스 구현.
// wpn_staff 등 마법 장비용 스탯 보너스(INT +1 등) 처리 파라미터 WeaponItem/ArmorItem에 주입.

#ifndef CONCRETE_ITEMS_HPP
#define CONCRETE_ITEMS_HPP

#include "model/Equipment.hpp"
#include "model/Character.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include <algorithm>

namespace crawl {

// 1. 소모성 아이템 추상 베이스 클래스
class ConsumableItem : public Item {
public:
    ConsumableItem(std::string id, int goldValue)
        : m_id(std::move(id)), m_goldValue(goldValue) {}

    ~ConsumableItem() override = default;

    std::string getId() const override { return m_id; }
    std::string getName() const override {
        return LocalizationManager::getInstance().getContent("ITEM", m_id, "NAME");
    }
    int getGoldValue() const override { return m_goldValue; }
    std::string getDescription() const override {
        return LocalizationManager::getInstance().getContent("ITEM", m_id, "DESC");
    }
    bool isEquipment() const override { return false; }

    // [v0.8.0] 소모품 고유 효과 처리 순수 가상 함수
    virtual void applyEffect(Character& target,
                             std::vector<std::shared_ptr<Character>>& party,
                             std::vector<std::string>& logOutput) = 0;

private:
    std::string m_id;
    int m_goldValue;
};

// 1.1 치유 물약 (pot_heal): HP 2d4 + 2 치유
class HealPotionItem : public ConsumableItem {
public:
    HealPotionItem() : ConsumableItem("pot_heal", 15) {}
    
    void applyEffect(Character& target, std::vector<std::shared_ptr<Character>>& /*party*/, std::vector<std::string>& logOutput) override {
        int healAmount = SessionRng::global().rollDie(4) + SessionRng::global().rollDie(4) + 2;
        
        target.heal(healAmount);
        logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_HEAL", {
            {"target", target.getName()}, {"item", getName()}, {"amount", std::to_string(healAmount)}}));
    }
};

// 1.2 고급 치유 물약 (pot_greater_heal): HP 4d4 + 4 치유
class GreaterHealPotionItem : public ConsumableItem {
public:
    GreaterHealPotionItem() : ConsumableItem("pot_greater_heal", 40) {}

    void applyEffect(Character& target, std::vector<std::shared_ptr<Character>>& /*party*/, std::vector<std::string>& logOutput) override {
        int healAmount = SessionRng::global().rollDie(4) + SessionRng::global().rollDie(4) +
                         SessionRng::global().rollDie(4) + SessionRng::global().rollDie(4) + 4;

        target.heal(healAmount);
        logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_HEAL", {
            {"target", target.getName()}, {"item", getName()}, {"amount", std::to_string(healAmount)}}));
    }
};

// 1.3 마나 물약 (pot_mana): 주문 슬롯 1개 회복
class ManaPotionItem : public ConsumableItem {
public:
    ManaPotionItem() : ConsumableItem("pot_mana", 30) {}

    void applyEffect(Character& target, std::vector<std::shared_ptr<Character>>& /*party*/, std::vector<std::string>& logOutput) override {
        if (target.getClass() != CharacterClass::MAGE && target.getClass() != CharacterClass::CLERIC) {
            logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_INVALID_CASTER", {
                {"target", target.getName()}, {"item", getName()}}));
            return;
        }
        
        int prevSlots = target.getSpellSlots();
        target.recoverSpellSlot(1);
        int diff = target.getSpellSlots() - prevSlots;

        if (diff > 0) {
            logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_SLOT_RECOVERED", {
                {"target", target.getName()}, {"item", getName()}}));
        } else {
            logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_SLOT_FULL", {
                {"target", target.getName()}}));
        }
    }
};

// 1.4 힘의 물약 (pot_strength): 전투 중 STR +3 증가 (5턴)
class StrengthPotionItem : public ConsumableItem {
public:
    StrengthPotionItem() : ConsumableItem("pot_strength", 25) {}

    void applyEffect(Character& target, std::vector<std::shared_ptr<Character>>& /*party*/, std::vector<std::string>& logOutput) override {
        target.applyStrBuff(3, 5); // 5턴 지속 힘 +3 버프
        logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_BUFF", {
            {"target", target.getName()}, {"item", getName()}}));
    }
};

// 1.5 민첩의 물약 (pot_dexterity): 전투 중 DEX +3 증가 (5턴)
class DexterityPotionItem : public ConsumableItem {
public:
    DexterityPotionItem() : ConsumableItem("pot_dexterity", 25) {}

    void applyEffect(Character& target, std::vector<std::shared_ptr<Character>>& /*party*/, std::vector<std::string>& logOutput) override {
        target.applyDexBuff(3, 5); // 5턴 지속 민첩 +3 버프
        logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_BUFF", {
            {"target", target.getName()}, {"item", getName()}}));
    }
};

// 1.6 해독 스크롤 (scr_cure): 독 상태 해제
class CureScrollItem : public ConsumableItem {
public:
    CureScrollItem() : ConsumableItem("scr_cure", 20) {}

    void applyEffect(Character& target, std::vector<std::shared_ptr<Character>>& /*party*/, std::vector<std::string>& logOutput) override {
        if (target.getPoisonTurns() > 0) {
            target.setPoison(0);
            logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_CURED", {
                {"target", target.getName()}, {"item", getName()}}));
        } else {
            logOutput.push_back(LocalizationManager::getInstance().format("ITEM_LOG_NOT_POISONED", {
                {"target", target.getName()}}));
        }
    }
};


// 2. 무기류 장비 구체 클래스 (예: 단검, 롱소드, 마법 지팡이 등)
class WeaponItem : public Equipment {
public:
    WeaponItem(std::string id, int goldValue, int diceCount, int diceSides,
               std::string bonusStat = "", int bonusVal = 0,
               DamageType damageType = DamageType::SLASHING)
        : m_id(std::move(id)), m_goldValue(goldValue),
          m_diceCount(diceCount), m_diceSides(diceSides), m_bonusStat(std::move(bonusStat)), m_bonusVal(bonusVal),
          m_damageType(damageType) {}

    ~WeaponItem() override = default;

    std::string getId() const override { return m_id; }
    std::string getName() const override {
        return LocalizationManager::getInstance().getContent("ITEM", m_id, "NAME");
    }
    int getGoldValue() const override { return m_goldValue; }
    std::string getDescription() const override {
        return LocalizationManager::getInstance().getContent("ITEM", m_id, "DESC");
    }

    EquipSlot getSlot() const override { return EquipSlot::WEAPON; }
    int getAcBonus() const override { return 0; }
    int getDamageDiceCount() const override { return m_diceCount; }
    int getDamageDiceSides() const override { return m_diceSides; }
    DamageType getDamageType() const override { return m_damageType; }
    
    // [v0.8.0] 장비 자체의 스탯 증가 보정 반환 (예: wpn_staff -> intelligence +1)
    int getStatBonus(const std::string& statName) const override {
        return (statName == m_bonusStat) ? m_bonusVal : 0;
    }

private:
    std::string m_id;
    int m_goldValue;
    int m_diceCount;
    int m_diceSides;
    std::string m_bonusStat;
    int m_bonusVal;
    DamageType m_damageType;
};

// 3. 방어구류 장비 구체 클래스 (갑옷, 방패)
class ArmorItem : public Equipment {
public:
    ArmorItem(std::string id, int goldValue, EquipSlot slot, int acBonus,
              std::string bonusStat = "", int bonusVal = 0)
        : m_id(std::move(id)), m_goldValue(goldValue), m_slot(slot), m_acBonus(acBonus),
          m_bonusStat(std::move(bonusStat)), m_bonusVal(bonusVal) {}

    ~ArmorItem() override = default;

    std::string getId() const override { return m_id; }
    std::string getName() const override {
        return LocalizationManager::getInstance().getContent("ITEM", m_id, "NAME");
    }
    int getGoldValue() const override { return m_goldValue; }
    std::string getDescription() const override {
        return LocalizationManager::getInstance().getContent("ITEM", m_id, "DESC");
    }

    EquipSlot getSlot() const override { return m_slot; }
    int getAcBonus() const override { return m_acBonus; }
    int getDamageDiceCount() const override { return 0; }
    int getDamageDiceSides() const override { return 0; }
    DamageType getDamageType() const override { return DamageType::BLUDGEONING; }
    
    // [v0.8.0] 방어구 스탯 증가 보정 반환
    int getStatBonus(const std::string& statName) const override {
        return (statName == m_bonusStat) ? m_bonusVal : 0;
    }

private:
    std::string m_id;
    int m_goldValue;
    EquipSlot m_slot;
    int m_acBonus;
    std::string m_bonusStat;
    int m_bonusVal;
};

} // namespace crawl

#endif // CONCRETE_ITEMS_HPP
