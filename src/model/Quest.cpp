#include "model/Quest.hpp"
#include "core/LocalizationManager.hpp"

#include <algorithm>
#include <stdexcept>

namespace crawl {

Quest::Quest(std::string id, std::string name, std::string desc, QuestType type,
             std::string targetId, int targetCount, int goldReward, int xpReward,
             std::vector<std::string> rewardItemIds, int targetFloor)
    : m_id(std::move(id)),
      m_name(std::move(name)),
      m_desc(std::move(desc)),
      m_type(type),
      m_targetId(std::move(targetId)),
      m_targetCount(std::max(1, targetCount)),
      m_goldReward(std::max(0, goldReward)),
      m_xpReward(std::max(0, xpReward)),
      m_targetFloor(std::clamp(targetFloor, 1, 5)),
      m_rewardItemIds(std::move(rewardItemIds)) {}

void Quest::updateProgress(const std::string& id, int count) {
    if (m_isCompleted || count <= 0) return;
    if (id != m_targetId) return;
    m_currentCount = std::min(m_targetCount, m_currentCount + count);
    if (m_currentCount >= m_targetCount) m_readyToReport = true;
}

void Quest::setCurrentCount(int count) {
    m_currentCount = std::clamp(count, 0, m_targetCount);
    if (m_currentCount >= m_targetCount) m_readyToReport = true;
}

bool Quest::checkCompletion() const {
    return m_readyToReport || m_currentCount >= m_targetCount;
}

bool Quest::markObjectiveComplete() {
    if (m_isCompleted) return false;
    m_currentCount = m_targetCount;
    m_readyToReport = true;
    return true;
}

bool Quest::isReadyToReport() const { return !m_isCompleted && checkCompletion(); }
std::string Quest::getId() const { return m_id; }

std::string Quest::getName() const {
    const auto localized = LocalizationManager::getInstance().getContent("QUEST", m_id, "NAME");
    return localized == ("QUEST_" + m_id + "_NAME") ? m_name : localized;
}

std::string Quest::getDescription() const {
    const auto localized = LocalizationManager::getInstance().getContent("QUEST", m_id, "DESC");
    return localized == ("QUEST_" + m_id + "_DESC") ? m_desc : localized;
}

QuestType Quest::getType() const { return m_type; }
std::string Quest::getTargetId() const { return m_targetId; }
int Quest::getTargetCount() const { return m_targetCount; }
int Quest::getCurrentCount() const { return m_currentCount; }
int Quest::getGoldReward() const { return m_goldReward; }
int Quest::getXpReward() const { return m_xpReward; }
int Quest::getTargetFloor() const { return m_targetFloor; }
bool Quest::isCompleted() const { return m_isCompleted; }
void Quest::setCompleted(bool completed) { m_isCompleted = completed; }
const std::vector<std::string>& Quest::getRewardItemIds() const { return m_rewardItemIds; }

std::vector<std::string> Quest::getCanonicalIds() {
    return {
        "qst_01_relic", "qst_02_warden", "qst_03_scout", "qst_04_hydra",
        "qst_05_phylactery", "qst_06_lich", "qst_07_forge_core", "qst_08_elder_wyrm"
    };
}

std::vector<std::string> Quest::getOfferableIds() { return getCanonicalIds(); }

std::shared_ptr<Quest> Quest::createCanonical(const std::string& id) {
    if (id == "qst_01_relic") {
        return std::make_shared<Quest>(
            id, "Sun Chalice Recovery", "Recover the Sun Chalice on Floor 1.",
            QuestType::RETRIEVE_KEY_ITEM, "key_sun_chalice", 1, 140, 120,
            std::vector<std::string>{"pot_greater_heal", "acc_iron_ring"}, 1);
    }
    if (id == "qst_02_warden") {
        return std::make_shared<Quest>(
            id, "Crypt Warden Bounty", "Defeat the Crypt Warden on Floor 1.",
            QuestType::DEFEAT_BOSS, "mon_crypt_warden", 1, 220, 200,
            std::vector<std::string>{"wpn_halberd"}, 1);
    }
    if (id == "qst_03_scout") {
        return std::make_shared<Quest>(
            id, "Missing Guild Scout", "Find Scout Ella in the Drowned Cistern (Floor 2).",
            QuestType::FIND_NPC, "npc_missing_scout", 1, 260, 260,
            std::vector<std::string>{"pot_panacea", "wpn_frost_wand"}, 2);
    }
    if (id == "qst_04_hydra") {
        return std::make_shared<Quest>(
            id, "Terror of the Cistern", "Slay the Bog Hydra on Floor 2.",
            QuestType::DEFEAT_BOSS, "mon_bog_hydra", 1, 340, 350,
            std::vector<std::string>{"acc_shadow_cloak"}, 2);
    }
    if (id == "qst_05_phylactery") {
        return std::make_shared<Quest>(
            id, "Cursed Phylactery", "Retrieve the Bone Phylactery on Floor 3.",
            QuestType::RETRIEVE_KEY_ITEM, "key_bone_phylactery", 1, 380, 420,
            std::vector<std::string>{"wpn_sunmace"}, 3);
    }
    if (id == "qst_06_lich") {
        return std::make_shared<Quest>(
            id, "Lord of the Ossuary", "Destroy the Ossuary Lich on Floor 3.",
            QuestType::DEFEAT_BOSS, "mon_ossuary_lich", 1, 500, 560,
            std::vector<std::string>{"arm_archmage_robe"}, 3);
    }
    if (id == "qst_07_forge_core") {
        return std::make_shared<Quest>(
            id, "Colossus of the Foundry", "Shatter the Iron Colossus on Floor 4.",
            QuestType::DEFEAT_BOSS, "mon_iron_colossus", 1, 680, 760,
            std::vector<std::string>{"shd_aegis", "pot_elixir"}, 4);
    }
    if (id == "qst_08_elder_wyrm") {
        return std::make_shared<Quest>(
            id, "Bane of the Elder Wyrm", "Confront and slay the Elder Wyrm on Floor 5.",
            QuestType::DEFEAT_BOSS, "mon_elder_wyrm", 1, 1500, 1500,
            std::vector<std::string>{"wpn_dragonbane", "arm_dragonscale"}, 5);
    }
    return nullptr;
}

bool Quest::matchesCanonicalDefinition() const {
    const auto canonical = createCanonical(m_id);
    return canonical && canonical->m_type == m_type && canonical->m_targetId == m_targetId &&
           canonical->m_targetCount == m_targetCount && canonical->m_targetFloor == m_targetFloor;
}

nlohmann::json Quest::toJson() const {
    return nlohmann::json{
        {"id", m_id},
        {"currentCount", m_currentCount},
        {"isCompleted", m_isCompleted},
        {"readyToReport", m_readyToReport}
    };
}

std::shared_ptr<Quest> Quest::fromJson(const nlohmann::json& j) {
    if (!j.is_object() || !j.contains("id")) throw std::runtime_error("invalid quest json");
    auto q = createCanonical(j.at("id").get<std::string>());
    if (!q) throw std::runtime_error("unknown canonical quest id");
    q->m_currentCount = std::clamp(j.value("currentCount", 0), 0, q->m_targetCount);
    q->m_isCompleted = j.value("isCompleted", false);
    q->m_readyToReport = j.value("readyToReport", q->m_currentCount >= q->m_targetCount);
    return q;
}

} // namespace crawl
