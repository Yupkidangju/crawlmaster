#ifndef COMBAT_ACTION_RULES_HPP
#define COMBAT_ACTION_RULES_HPP

namespace crawl {

class Character;
class ConsumableItem;

class CombatActionRules {
public:
    static bool canUseConsumable(const ConsumableItem& item, const Character& target);
};

} // namespace crawl

#endif // COMBAT_ACTION_RULES_HPP
