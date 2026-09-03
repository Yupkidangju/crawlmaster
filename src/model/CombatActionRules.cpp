#include "model/CombatActionRules.hpp"

#include "model/Character.hpp"
#include "model/ConcreteItems.hpp"

namespace crawl {

bool CombatActionRules::canUseConsumable(const ConsumableItem& item, const Character& target) {
    if (target.isDead()) return false;
    const std::string id = item.getId();
    if (id == "pot_heal" || id == "pot_greater_heal") return target.getHp() < target.getMaxHp();
    if (id == "pot_mana") {
        const bool spellcaster = target.getClass() == CharacterClass::MAGE ||
                                 target.getClass() == CharacterClass::CLERIC;
        return spellcaster && target.getSpellSlots() < target.getMaxSpellSlots();
    }
    if (id == "scr_cure") return target.getPoisonTurns() > 0;
    if (id == "pot_strength" || id == "pot_dexterity") return true;
    return false;
}

} // namespace crawl
