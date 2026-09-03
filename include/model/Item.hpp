// [v0.3.0] Item.hpp 신규 작성
// 던전 및 상점에서 사용될 모든 게임 아이템의 기반이 되는 추상 인터페이스 정의.

#ifndef ITEM_HPP
#define ITEM_HPP

#include <string>
#include <memory>

namespace crawl {

// Item 클래스: 모든 소모품 및 장비의 부모가 되는 순수 가상 추상 클래스
class Item {
public:
    virtual ~Item() = default;

    // 아이템의 고유 ID 반환 (예: "wpn_dagger", "pot_heal")
    virtual std::string getId() const = 0;

    // 아이템의 출력 이름 반환 (예: "단검", "치유 물약")
    virtual std::string getName() const = 0;

    // 아이템의 상점 가치 반환 (골드 단위)
    virtual int getGoldValue() const = 0;

    // 아이템 상세 설명 반환
    virtual std::string getDescription() const = 0;

    // 아이템이 장비 가능한 장구류인지 여부 판정
    virtual bool isEquipment() const = 0;
};

} // namespace crawl

#endif // ITEM_HPP
