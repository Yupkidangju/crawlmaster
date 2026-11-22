#include "model/CombatActionRules.hpp"
#include "model/Character.hpp"
#include "model/ConcreteItems.hpp"

namespace crawl {

bool CombatActionRules::canUseConsumable(const ConsumableItem& item, const Character& target) {
    const std::string id = item.getId();
    if (item.isOffensiveBomb()) return true;
    if (id == "pot_elixir") {
        return target.isDead() || target.getHp() < target.getMaxHp() ||
               target.hasAnyDebuff() || target.getSpellSlots() < target.getMaxSpellSlots();
    }
    if (target.isDead()) return false;
    if (id == "pot_heal" || id == "pot_greater_heal") {
        return target.getHp() < target.getMaxHp();
    }
    if (id == "pot_mana" || id == "pot_greater_mana") {
        return target.getMaxSpellSlots() > 0 && target.getSpellSlots() < target.getMaxSpellSlots();
    }
    if (id == "scr_cure") {
        return target.hasAnyDebuff();
    }
    if (id == "pot_panacea") {
        return target.hasAnyDebuff() || target.getHp() < target.getMaxHp();
    }
    if (id == "pot_strength" || id == "pot_dexterity") {
        return true;
    }
    return true;
}

bool CombatActionRules::canUseConsumableOnParty(
    const ConsumableItem& item, const std::vector<std::shared_ptr<Character>>& party) {
    if (item.isOffensiveBomb()) return true;
    for (const auto& m : party) {
        if (m && canUseConsumable(item, *m)) return true;
    }
    return false;
}

} // namespace crawl
