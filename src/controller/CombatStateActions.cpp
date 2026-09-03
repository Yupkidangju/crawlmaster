#include "controller/CombatState.hpp"

#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "model/CombatActionRules.hpp"
#include "model/ConcreteItems.hpp"
#include "model/Party.hpp"
#include "model/Skill.hpp"

namespace crawl {

std::vector<int> CombatState::getConsumableInventoryIndices() const {
    std::vector<int> indices;
    const auto& inventory = m_game.getParty().getInventory();
    for (std::size_t index = 0; index < inventory.size(); ++index) {
        if (inventory[index] && !inventory[index]->isEquipment()) {
            indices.push_back(static_cast<int>(index));
        }
    }
    return indices;
}

void CombatState::confirmSelectedItem() {
    Party& party = m_game.getParty();
    const auto& inventory = party.getInventory();
    auto target = party.getMember(m_selectedAllyIndex);
    if (m_selectedInventoryIndex < 0 ||
        m_selectedInventoryIndex >= static_cast<int>(inventory.size()) || !target) return;

    auto selectedItem = std::dynamic_pointer_cast<ConsumableItem>(
        inventory[static_cast<std::size_t>(m_selectedInventoryIndex)]);
    if (!selectedItem || !CombatActionRules::canUseConsumable(*selectedItem, *target)) return;

    std::vector<std::string> useLogs;
    std::vector<std::shared_ptr<Character>> partyMembers;
    for (int index = 0; index < party.getMemberCount(); ++index) {
        partyMembers.push_back(party.getMember(index));
    }
    selectedItem->applyEffect(*target, partyMembers, useLogs);
    for (const auto& message : useLogs) addLog(message);
    party.removeItem(m_selectedInventoryIndex);

    m_isConfirmingItem = false;
    m_selectedInventoryIndex = -1;
    nextTurn();
}

bool CombatState::executePendingAllySkill() {
    const auto& currentEntity = m_turnOrder[m_currentTurnIdx];
    auto actor = m_game.getParty().getMember(currentEntity.index);
    if (!actor || m_pendingSkillIndex < 0 ||
        m_pendingSkillIndex >= static_cast<int>(actor->getSkills().size())) return false;

    auto skill = actor->getSkills()[static_cast<std::size_t>(m_pendingSkillIndex)];
    std::vector<std::shared_ptr<Character>> allies;
    for (int index = 0; index < m_game.getParty().getMemberCount(); ++index) {
        allies.push_back(m_game.getParty().getMember(index));
    }
    std::vector<std::string> logs;
    const bool committed = skill->execute(*actor, allies, m_foes, m_selectedAllyIndex, logs);
    for (const auto& message : logs) addLog(message);
    if (!committed) {
        addLog(LocalizationManager::getInstance().get("COMBAT_LOG_NO_VALID_ACTION_TARGET"));
        return false;
    }
    m_isSelectingSkillTarget = false;
    m_isConfirmingSkillTarget = false;
    m_pendingSkillIndex = -1;
    return true;
}

} // namespace crawl
