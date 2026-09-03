// [v0.4.0] Quest.cpp 신규 작성
// 퀘스트 진행도 갱신, 수집형 타겟 수량 비교 및 nlohmann/json 영속 직렬화 구현을 다룬다.

#include "model/Quest.hpp"
#include "core/LocalizationManager.hpp"
#include <stdexcept>

namespace crawl {

Quest::Quest(std::string id, std::string name, std::string desc, QuestType type,
             std::string targetId, int targetCount, int goldReward, int xpReward,
             std::vector<std::string> rewardItemIds)
    : m_id(std::move(id)), m_name(std::move(name)), m_desc(std::move(desc)), m_type(type),
      m_targetId(std::move(targetId)), m_targetCount(targetCount), m_currentCount(0),
      m_goldReward(goldReward), m_xpReward(xpReward), m_isCompleted(false),
      m_rewardItemIds(std::move(rewardItemIds)) {}

void Quest::updateProgress(const std::string& id, int count) {
    // 적 처치(KILL) 타입의 진행도 자동 가산 처리
    if (m_type == QuestType::KILL && m_targetId == id) {
        m_currentCount = std::min(m_targetCount, m_currentCount + count);
    }
}

void Quest::setCurrentCount(int count) {
    m_currentCount = std::min(m_targetCount, count);
}

bool Quest::checkCompletion() const {
    return m_currentCount >= m_targetCount;
}

std::string Quest::getId() const { return m_id; }
std::string Quest::getName() const { return LocalizationManager::getInstance().get(m_name); }
std::string Quest::getDescription() const { return LocalizationManager::getInstance().get(m_desc); }
QuestType Quest::getType() const { return m_type; }
std::string Quest::getTargetId() const { return m_targetId; }
int Quest::getTargetCount() const { return m_targetCount; }
int Quest::getCurrentCount() const { return m_currentCount; }
int Quest::getGoldReward() const { return m_goldReward; }
int Quest::getXpReward() const { return m_xpReward; }
bool Quest::isCompleted() const { return m_isCompleted; }
void Quest::setCompleted(bool completed) { m_isCompleted = completed; }
const std::vector<std::string>& Quest::getRewardItemIds() const { return m_rewardItemIds; }

std::vector<std::string> Quest::getCanonicalIds() {
    return {"qst_clear_kobolds", "qst_collect_maces", "qst_hunt_spiders"};
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
    return nullptr;
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
    j["rewardItemIds"] = m_rewardItemIds;
    return j;
}

std::shared_ptr<Quest> Quest::fromJson(const nlohmann::json& j, int schemaVersion) {
    if (!j.is_object()) throw std::runtime_error("quest는 객체여야 합니다.");
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
    if (j.contains("rewardItemIds") &&
        j.at("rewardItemIds").get<std::vector<std::string>>() != quest->getRewardItemIds()) {
        throw std::runtime_error("quest rewardItemIds가 canonical registry와 다릅니다.");
    }

    const int currentCount = j.value(currentCountKey, 0);
    if (currentCount < 0 || currentCount > quest->getTargetCount()) {
        throw std::runtime_error("quest currentCount 범위를 벗어났습니다.");
    }
    if (j.value(completedKey, false)) {
        throw std::runtime_error("active quest는 completed 상태일 수 없습니다.");
    }
    quest->m_currentCount = currentCount;
    return quest;
}

} // namespace crawl
