// [v0.3.0] Equipment.hpp 신규 작성
// 무기, 갑옷, 방패 등 캐릭터가 장착 가능한 장구류의 스펙과 AC 보너스 및 피해 주사위를 조회하는 추상 클래스 정의.

#ifndef EQUIPMENT_HPP
#define EQUIPMENT_HPP

#include "Item.hpp"

namespace crawl {

enum class DamageType {
    SLASHING,
    PIERCING,
    BLUDGEONING
};

// 장착 슬롯 정의
enum class EquipSlot {
    WEAPON,     // 무기 (주손)
    ARMOR,      // 갑옷 (몸통)
    SHIELD,     // 방패 (보조손)
    ACCESSORY   // 장신구
};

// Equipment 클래스: Item을 상속받으며 D&D 전투 판정에 쓰일 공격/방어 데이터 제공
class Equipment : public Item {
public:
    ~Equipment() override = default;

    bool isEquipment() const override { return true; }

    // 장비가 장착되는 슬롯 반환
    virtual EquipSlot getSlot() const = 0;

    // 장비가 제공하는 AC(방어 등급) 보너스 반환
    virtual int getAcBonus() const = 0;

    // 무기일 경우 공격 피해 주사위 개수 반환 (예: 1d8일 시 1 반환)
    virtual int getDamageDiceCount() const = 0;

    // 무기일 경우 공격 피해 주사위 면수 반환 (예: 1d8일 시 8 반환)
    virtual int getDamageDiceSides() const = 0;
    virtual DamageType getDamageType() const = 0;

    // 특정 능력치 보너스 반환 (기본값은 0)
    virtual int getStatBonus(const std::string& statName) const = 0;
};

} // namespace crawl

#endif // EQUIPMENT_HPP
