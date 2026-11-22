#ifndef ITEM_FACTORY_HPP
#define ITEM_FACTORY_HPP

#include "model/Equipment.hpp"
#include "model/Item.hpp"
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace crawl {

class SessionRng;

class ItemFactory {
public:
    static std::shared_ptr<Item> createItem(const std::string& id);
    static std::shared_ptr<Item> createItemFromJson(const nlohmann::json& j);
    static std::vector<std::shared_ptr<Item>> getShopCatalog(int deepestFloor = 1);
    static std::vector<std::string> getRegisteredIds();

    static ItemAffix rollAffix(ItemRarity rarity, SessionRng& rng);
    static std::shared_ptr<Item> rollChestLoot(int floorNumber, SessionRng& rng);
};

} // namespace crawl

#endif // ITEM_FACTORY_HPP
