// [v0.3.0] Party.hpp 신규 작성
// 최대 4인의 캐릭터 멤버, 공용 인벤토리/소지 골드를 트래킹하고 JSON 파일 세이브/로드를 제어하는 파티 매니저 헤더 정의.

#ifndef PARTY_HPP
#define PARTY_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <unordered_set>
#include "core/Persistence.hpp"
#include "model/Character.hpp"
#include "model/Item.hpp"
#include "model/Quest.hpp"
#include "model/DungeonWorld.hpp"

namespace crawl {

class PartyCheckpoint {
private:
    friend class Party;
    int gold = 0;
    std::vector<std::string> inventoryIds;
    std::vector<std::shared_ptr<Character>> members;
    std::vector<std::shared_ptr<Quest>> quests;
    std::unordered_set<std::string> completedQuestIds;
    std::unordered_set<std::string> keyItems;
    DungeonWorld world;
    bool campaignCompleted = false;
    std::uint32_t lastSessionSeed = 0;
    std::uint64_t sessionRngDrawCount = 0;
    bool activeSaveSession = false;
    bool recoveryPending = false;
    int loadedSchemaVersion = 4;
    std::uint32_t globalRngSeed = 0;
    std::uint64_t globalRngDrawCount = 0;
};

// Party 클래스: 활성 파티 멤버 리스트 및 공용 인벤토리, 세이브파일 입출력 총괄
class Party {
public:
    Party();
    ~Party() = default;

    // [v0.9.4] 테스트 실행 시 사용자 세이브와 분리할 수 있도록 기본 저장 경로를 프로세스 단위로 지정한다.
    static void setDefaultSavePath(const std::string& filePath);
    static const std::string& getDefaultSavePath();
    static bool hasRecoverableSave(const std::string& filePath = getDefaultSavePath());

    // 파티원 관리 (최대 4인)
    bool addMember(std::shared_ptr<Character> member);
    void removeMember(int index);
    
    const std::vector<std::shared_ptr<Character>>& getMembers() const;
    std::shared_ptr<Character> getMember(int index) const;
    int getMemberCount() const;

    // 골드 소유 및 차감/지급
    int getGold() const;
    void addGold(int amount);
    bool spendGold(int amount);

    // 공용 인벤토리 아이템 관리
    const std::vector<std::shared_ptr<Item>>& getInventory() const;
    void addItem(std::shared_ptr<Item> item);
    void insertItem(int index, std::shared_ptr<Item> item);
    void removeItem(int index);

    // 활성 퀘스트 임무 관리
    const std::vector<std::shared_ptr<Quest>>& getActiveQuests() const;
    void acceptQuest(std::shared_ptr<Quest> quest);
    void completeQuest(const std::string& questId);
    void abandonQuest(const std::string& questId);
    bool hasQuest(const std::string& questId) const;
    void updateQuestKillProgress(const std::string& monsterId, int count);
    void updateQuestCollectProgress();
    bool markQuestObjectiveComplete(const std::string& questId);
    std::shared_ptr<Quest> getQuest(const std::string& questId) const;

    bool addKeyItem(const std::string& itemId);
    bool removeKeyItem(const std::string& itemId);
    bool hasKeyItem(const std::string& itemId) const;
    const std::unordered_set<std::string>& getKeyItems() const;

    DungeonWorld& getWorld();
    const DungeonWorld& getWorld() const;

    // 세이브 & 로드 정책 (save.json 연동)
    PersistenceResult saveToFile(const std::string& filePath = getDefaultSavePath());
    PersistenceResult loadFromFile(const std::string& filePath = getDefaultSavePath());

    // 메모리 기본값과 명시적인 신규 게임 저장은 서로 다른 명령이다.
    void resetToDefault();
    PersistenceResult startNewGame(const std::string& filePath = getDefaultSavePath());

    bool isQuestCompleted(const std::string& questId) const;
    const std::unordered_set<std::string>& getCompletedQuestIds() const;
    bool isCampaignCompleted() const;
    void setCampaignCompleted(bool completed);
    std::uint32_t getLastSessionSeed() const;
    void setLastSessionSeed(std::uint32_t seed);
    std::uint64_t getSessionRngDrawCount() const;
    bool hasActiveSaveSession() const;
    bool isRecoveryPending() const;
    void markRecoveryPending();
    bool needsSaveMigration() const;
    PartyCheckpoint captureCheckpoint() const;
    void restoreCheckpoint(const PartyCheckpoint& checkpoint);

private:
    void validateState() const;
    std::vector<std::shared_ptr<Character>> m_members;  // 최대 4인 파티원 멤버 배열
    std::vector<std::shared_ptr<Item>> m_inventory;     // 파티 공용 가방 인벤토리
    std::vector<std::shared_ptr<Quest>> m_activeQuests; // 현재 활성 수락된 퀘스트 리스트
    std::unordered_set<std::string> m_completedQuestIds;
    std::unordered_set<std::string> m_keyItems;
    DungeonWorld m_world;
    int m_gold;                                         // 파티 총 보유 골드량
    bool m_campaignCompleted = false;
    std::uint32_t m_lastSessionSeed = 0;
    std::uint64_t m_sessionRngDrawCount = 0;
    bool m_hasActiveSaveSession = false;
    bool m_recoveryPending = false;
    int m_loadedSchemaVersion = 4;
    inline static std::string s_defaultSavePath; // 테스트에서만 명시 경로를 주입한다.
};

} // namespace crawl

#endif // PARTY_HPP
