#ifndef COMBAT_ACTION_RULES_HPP
#define COMBAT_ACTION_RULES_HPP

#include <memory>
#include <vector>

namespace crawl {

class Character;
class ConsumableItem;

class CombatActionRules {
public:
    static bool canUseConsumable(const ConsumableItem& item, const Character& target);
    static bool canUseConsumableOnParty(const ConsumableItem& item,
                                        const std::vector<std::shared_ptr<Character>>& party);
};

} // namespace crawl

#endif // COMBAT_ACTION_RULES_HPP
