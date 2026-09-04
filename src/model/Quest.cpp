// [v0.4.0] Quest.cpp 신규 작성
// 퀘스트 진행도 갱신, 수집형 타겟 수량 비교 및 nlohmann/json 영속 직렬화 구현을 다룬다.

#include "model/Quest.hpp"
#include "core/LocalizationManager.hpp"
#include <limits>
#include <stdexcept>

namespace crawl {

Quest::Quest(std::string id, std::string name, std::string desc, QuestType type,
             std::string targetId, int targetCount, int goldReward, int xpReward,
             std::vector<std::string> rewardItemIds, int targetFloor)
    : m_id(std::move(id)), m_name(std::move(name)), m_desc(std::move(desc)), m_type(type),
      m_targetId(std::move(targetId)), m_targetCount(targetCount), m_currentCount(0),
      m_goldReward(goldReward), m_xpReward(xpReward), m_isCompleted(false),
      m_targetFloor(targetFloor), m_rewardItemIds(std::move(rewardItemIds)) {}

void Quest::updateProgress(const std::string& id, int count) {
    // 적 처치(KILL) 타입의 진행도 자동 가산 처리
    if (m_type == QuestType::KILL && m_targetId == id) {
        if (count <= 0) return;
        const int remaining = m_targetCount - m_currentCount;
        m_currentCount += std::min(count, remaining);
    }
}

void Quest::setCurrentCount(int count) {
    m_currentCount = std::clamp(count, 0, m_targetCount);
}

bool Quest::checkCompletion() const {
    if (m_type == QuestType::KILL || m_type == QuestType::COLLECT) {
        return m_currentCount >= m_targetCount;
    }
    return m_readyToReport;
}

bool Quest::markObjectiveComplete() {
    if (m_type == QuestType::KILL || m_type == QuestType::COLLECT || m_readyToReport) return false;
    m_readyToReport = true;
    m_currentCount = m_targetCount;
    return true;
}

bool Quest::isReadyToReport() const { return checkCompletion(); }

std::string Quest::getId() const { return m_id; }
std::string Quest::getName() const { return LocalizationManager::getInstance().get(m_name); }
std::string Quest::getDescription() const { return LocalizationManager::getInstance().get(m_desc); }
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
    return {"qst_clear_kobolds", "qst_collect_maces", "qst_hunt_spiders",
            "qst_recover_moon_seal", "qst_defeat_crypt_warden", "qst_find_missing_scout"};
}

std::vector<std::string> Quest::getOfferableIds() {
    return {"qst_recover_moon_seal", "qst_defeat_crypt_warden", "qst_find_missing_scout"};
}

std::shared_ptr<Quest> Quest::createCanonical(const std::string& id) {
    if (id == "qst_clear_kobolds") {
        return std::make_shared<Quest>(id, "QUEST_QST_CLEAR_KOBOLDS_NAME", "QUEST_QST_CLEAR_KOBOLDS_DESC", QuestType::KILL,
                                       "mon_kobold", 5, 50, 100,
                                       std::vector<std::string>{"pot_strength"});
    }
    if (id == "qst_collect_maces") {
        return std::make_shared<Quest>(id, "QUEST_QST_COLLECT_MACES_NAME", "QUEST_QST_COLLECT_MACES_DESC", QuestType::COLLECT,
                                       "wpn_mace", 2, 80, 150,
                                       std::vector<std::string>{"wpn_rapier"});
    }
    if (id == "qst_hunt_spiders") {
        return std::make_shared<Quest>(id, "QUEST_QST_HUNT_SPIDERS_NAME", "QUEST_QST_HUNT_SPIDERS_DESC", QuestType::KILL,
                                       "mon_giant_spider", 3, 100, 200,
                                       std::vector<std::string>{"scr_cure", "pot_greater_heal"});
    }
    if (id == "qst_recover_moon_seal") {
        return std::make_shared<Quest>(id, "QUEST_QST_RECOVER_MOON_SEAL_NAME", "QUEST_QST_RECOVER_MOON_SEAL_DESC",
                                       QuestType::RETRIEVE_KEY_ITEM, "key_moon_seal", 1, 75, 150,
                                       std::vector<std::string>{"pot_strength"}, 1);
    }
    if (id == "qst_defeat_crypt_warden") {
        return std::make_shared<Quest>(id, "QUEST_QST_DEFEAT_CRYPT_WARDEN_NAME", "QUEST_QST_DEFEAT_CRYPT_WARDEN_DESC",
                                       QuestType::DEFEAT_BOSS, "mon_crypt_warden", 1, 150, 300,
                                       std::vector<std::string>{"wpn_rapier"}, 2);
    }
    if (id == "qst_find_missing_scout") {
        return std::make_shared<Quest>(id, "QUEST_QST_FIND_MISSING_SCOUT_NAME", "QUEST_QST_FIND_MISSING_SCOUT_DESC",
                                       QuestType::FIND_NPC, "npc_missing_scout", 1, 200, 400,
                                       std::vector<std::string>{"scr_cure", "pot_greater_heal"}, 3);
    }
    return nullptr;
}

bool Quest::matchesCanonicalDefinition() const {
    const auto canonical = createCanonical(m_id);
    return canonical && canonical->m_name == m_name && canonical->m_desc == m_desc &&
           canonical->m_type == m_type && canonical->m_targetId == m_targetId &&
           canonical->m_targetCount == m_targetCount && canonical->m_goldReward == m_goldReward &&
           canonical->m_xpReward == m_xpReward && canonical->m_targetFloor == m_targetFloor &&
           canonical->m_rewardItemIds == m_rewardItemIds;
}

nlohmann::json Quest::toJson() const {
    nlohmann::json j;
    j["id"] = m_id;
    j["name"] = m_name;
    j["desc"] = m_desc;
    j["type"] = static_cast<int>(m_type);
    j["targetId"] = m_targetId;
    j["targetCount"] = m_targetCount;
    j["currentCount"] = m_currentCount;
    j["goldReward"] = m_goldReward;
    j["xpReward"] = m_xpReward;
    j["isCompleted"] = m_isCompleted;
    j["readyToReport"] = m_readyToReport;
    j["targetFloor"] = m_targetFloor;
    j["rewardItemIds"] = m_rewardItemIds;
    return j;
}

std::shared_ptr<Quest> Quest::fromJson(const nlohmann::json& j, int schemaVersion) {
    if (!j.is_object()) throw std::runtime_error("quest는 객체여야 합니다.");
    if (schemaVersion >= 4) {
        static const std::vector<std::string> required = {
            "id", "name", "desc", "type", "targetId", "targetCount", "currentCount",
            "goldReward", "xpReward", "isCompleted", "rewardItemIds", "readyToReport", "targetFloor"};
        for (const auto& key : required) {
            if (!j.contains(key)) throw std::runtime_error("v4 quest 필수 필드 누락: " + key);
        }
    }
    const std::string id = j.at("id").get<std::string>();
    auto quest = createCanonical(id);
    if (!quest) throw std::runtime_error("알 수 없는 canonical quest id: " + id);

    const bool canonicalShape = schemaVersion >= 2;
    const char* targetIdKey = canonicalShape ? "targetId" : "target_id";
    const char* targetCountKey = canonicalShape ? "targetCount" : "target_count";
    const char* currentCountKey = canonicalShape ? "currentCount" : "current_count";
    const char* goldRewardKey = canonicalShape ? "goldReward" : "gold_reward";
    const char* xpRewardKey = canonicalShape ? "xpReward" : "xp_reward";
    const char* completedKey = canonicalShape ? "isCompleted" : "is_completed";

    if (schemaVersion >= 4 &&
        (j.at("name").get<std::string>() != quest->m_name ||
         j.at("desc").get<std::string>() != quest->m_desc)) {
        throw std::runtime_error("quest localization key가 canonical registry와 다릅니다.");
    }

    if (j.contains("type") && j.at("type").get<int>() != static_cast<int>(quest->getType())) {
        throw std::runtime_error("quest type이 canonical registry와 다릅니다.");
    }
    if (j.contains(targetIdKey) && j.at(targetIdKey).get<std::string>() != quest->getTargetId()) {
        throw std::runtime_error("quest targetId가 canonical registry와 다릅니다.");
    }
    if (j.contains(targetCountKey) && j.at(targetCountKey).get<int>() != quest->getTargetCount()) {
        throw std::runtime_error("quest targetCount가 canonical registry와 다릅니다.");
    }
    if (j.contains(goldRewardKey) && j.at(goldRewardKey).get<int>() != quest->getGoldReward()) {
        throw std::runtime_error("quest goldReward가 canonical registry와 다릅니다.");
    }
    if (j.contains(xpRewardKey) && j.at(xpRewardKey).get<int>() != quest->getXpReward()) {
        throw std::runtime_error("quest xpReward가 canonical registry와 다릅니다.");
    }
    if ((schemaVersion >= 4 || j.contains("rewardItemIds")) &&
        j.at("rewardItemIds").get<std::vector<std::string>>() != quest->getRewardItemIds()) {
        throw std::runtime_error("quest rewardItemIds가 canonical registry와 다릅니다.");
    }
    if (schemaVersion >= 4 && j.contains("targetFloor") &&
        j.at("targetFloor").get<int>() != quest->getTargetFloor()) {
        throw std::runtime_error("quest targetFloor가 canonical registry와 다릅니다.");
    }

    const int currentCount = j.value(currentCountKey, 0);
    if (currentCount < 0 || currentCount > quest->getTargetCount()) {
        throw std::runtime_error("quest currentCount 범위를 벗어났습니다.");
    }
    if ((schemaVersion >= 4 ? j.at(completedKey).get<bool>() : j.value(completedKey, false))) {
        throw std::runtime_error("active quest는 completed 상태일 수 없습니다.");
    }
    quest->m_currentCount = currentCount;
    if (schemaVersion >= 4) {
        quest->m_readyToReport = j.at("readyToReport").get<bool>();
        if (quest->getType() != QuestType::KILL && quest->getType() != QuestType::COLLECT &&
            currentCount != (quest->m_readyToReport ? quest->getTargetCount() : 0)) {
            throw std::runtime_error("목적형 quest 진행도와 보고 상태가 일치하지 않습니다.");
        }
        if (quest->m_readyToReport) quest->m_currentCount = quest->m_targetCount;
    }
    return quest;
}

} // namespace crawl
