// [v0.4.0] Quest.hpp 신규 작성
// 퀘스트 정보 모델링 및 타입 정의, JSON 영속 저장을 위한 데이터 스펙을 구현한 헤더 정의.

#ifndef QUEST_HPP
#define QUEST_HPP

#include <string>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>

namespace crawl {

// 퀘스트 타입 정의
enum class QuestType {
    KILL,       // 적 처치 임무
    COLLECT,    // 아이템 수집 임무 (구세이브 호환)
    DEFEAT_BOSS,
    RETRIEVE_KEY_ITEM,
    FIND_NPC
};

// Quest 클래스: 개별 임무의 진행도, 보상, 타입 및 완수 플래그 관리
class Quest {
public:
    Quest(std::string id, std::string name, std::string desc, QuestType type,
          std::string targetId, int targetCount, int goldReward, int xpReward,
          std::vector<std::string> rewardItemIds = {}, int targetFloor = 0);
    ~Quest() = default;

    // 진행 수량 업데이트 시도
    void updateProgress(const std::string& id, int count);

    // 수집 대상 아이템 등 직접 진행도 조작 (체크용)
    void setCurrentCount(int count);

    // 퀘스트 성공 달성 요건 충족 유무 확인
    bool checkCompletion() const;
    bool markObjectiveComplete();
    bool isReadyToReport() const;

    // Getter 함수들
    std::string getId() const;
    std::string getName() const;
    std::string getDescription() const;
    QuestType getType() const;
    std::string getTargetId() const;
    int getTargetCount() const;
    int getCurrentCount() const;
    int getGoldReward() const;
    int getXpReward() const;
    int getTargetFloor() const;
    bool isCompleted() const;
    void setCompleted(bool completed);
    const std::vector<std::string>& getRewardItemIds() const;

    static std::vector<std::string> getCanonicalIds();
    static std::vector<std::string> getOfferableIds();
    static std::shared_ptr<Quest> createCanonical(const std::string& id);
    bool matchesCanonicalDefinition() const;

    // --- JSON 세이브 연동 직렬화/역직렬화 ---
    nlohmann::json toJson() const;
    static std::shared_ptr<Quest> fromJson(const nlohmann::json& j, int schemaVersion = 2);

private:
    std::string m_id;
    std::string m_name;
    std::string m_desc;
    QuestType m_type;
    std::string m_targetId;
    int m_targetCount;
    int m_currentCount;
    int m_goldReward;
    int m_xpReward;
    bool m_isCompleted;
    bool m_readyToReport = false;
    int m_targetFloor = 0;
    std::vector<std::string> m_rewardItemIds;
};

} // namespace crawl

#endif // QUEST_HPP
