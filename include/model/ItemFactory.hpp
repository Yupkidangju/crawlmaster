// [v0.3.0] ItemFactory.hpp 신규 작성
// 아이템 고유 ID를 전달받아 그에 대치되는 소모품/무기/갑옷 등의 구체 객체를 동적으로 할당 및 생성하는 팩토리 헤더 정의.

#ifndef ITEM_FACTORY_HPP
#define ITEM_FACTORY_HPP

#include "Item.hpp"
#include <memory>
#include <vector>

namespace crawl {

// ItemFactory 클래스: 전역 아이템 데이터베이스 보조 및 동적 생성 담당
class ItemFactory {
public:
    // 고유 ID에 따른 새 아이템 객체 생성 (std::shared_ptr 반환)
    static std::shared_ptr<Item> createItem(const std::string& id);

    // Town에서 직접 구매 가능한 기본 8종 카탈로그
    static std::vector<std::shared_ptr<Item>> getShopCatalog();
    static std::vector<std::string> getRegisteredIds();
};

} // namespace crawl

#endif // ITEM_FACTORY_HPP
