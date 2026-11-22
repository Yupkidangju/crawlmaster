#ifndef QUEST_HPP
#define QUEST_HPP

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace crawl {

enum class QuestType {
    KILL = 0,
    COLLECT = 1,
    DEFEAT_BOSS = 2,
    RETRIEVE_KEY_ITEM = 3,
    FIND_NPC = 4
};

class Quest {
public:
    Quest(std::string id, std::string name, std::string desc, QuestType type,
          std::string targetId, int targetCount, int goldReward, int xpReward,
          std::vector<std::string> rewardItemIds = {}, int targetFloor = 1);
    ~Quest() = default;

    void updateProgress(const std::string& id, int count);
    void setCurrentCount(int count);
    [[nodiscard]] bool checkCompletion() const;
    bool markObjectiveComplete();
    [[nodiscard]] bool isReadyToReport() const;

    [[nodiscard]] std::string getId() const;
    [[nodiscard]] std::string getName() const;
    [[nodiscard]] std::string getDescription() const;
    [[nodiscard]] QuestType getType() const;
    [[nodiscard]] std::string getTargetId() const;
    [[nodiscard]] int getTargetCount() const;
    [[nodiscard]] int getCurrentCount() const;
    [[nodiscard]] int getGoldReward() const;
    [[nodiscard]] int getXpReward() const;
    [[nodiscard]] int getTargetFloor() const;
    [[nodiscard]] bool isCompleted() const;
    void setCompleted(bool completed);
    [[nodiscard]] const std::vector<std::string>& getRewardItemIds() const;

    static std::vector<std::string> getCanonicalIds();
    static std::vector<std::string> getOfferableIds();
    static std::shared_ptr<Quest> createCanonical(const std::string& id);
    [[nodiscard]] bool matchesCanonicalDefinition() const;

    [[nodiscard]] nlohmann::json toJson() const;
    static std::shared_ptr<Quest> fromJson(const nlohmann::json& j);

private:
    std::string m_id;
    std::string m_name;
    std::string m_desc;
    QuestType m_type;
    std::string m_targetId;
    int m_targetCount;
    int m_currentCount = 0;
    int m_goldReward;
    int m_xpReward;
    bool m_isCompleted = false;
    bool m_readyToReport = false;
    int m_targetFloor = 1;
    std::vector<std::string> m_rewardItemIds;
};

} // namespace crawl

#endif // QUEST_HPP
