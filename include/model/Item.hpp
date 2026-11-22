#ifndef ITEM_HPP
#define ITEM_HPP

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace crawl {

enum class ItemRarity {
    COMMON = 0,
    UNCOMMON = 1,
    RARE = 2,
    EPIC = 3,
    LEGENDARY = 4
};

class Item {
public:
    virtual ~Item() = default;

    [[nodiscard]] virtual std::string getId() const = 0;
    [[nodiscard]] virtual std::string getName() const = 0;
    [[nodiscard]] virtual int getGoldValue() const = 0;
    [[nodiscard]] virtual std::string getDescription() const = 0;
    [[nodiscard]] virtual bool isEquipment() const = 0;
    [[nodiscard]] virtual ItemRarity getRarity() const { return ItemRarity::COMMON; }
    [[nodiscard]] virtual std::shared_ptr<Item> clone() const = 0;
    [[nodiscard]] virtual nlohmann::json toJson() const {
        return nlohmann::json{{"id", getId()}};
    }
};

} // namespace crawl

#endif // ITEM_HPP
