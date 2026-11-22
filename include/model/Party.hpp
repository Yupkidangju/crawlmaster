#ifndef PARTY_HPP
#define PARTY_HPP

#include "core/Persistence.hpp"
#include "model/Bestiary.hpp"
#include "model/Character.hpp"
#include "model/DungeonWorld.hpp"
#include "model/Item.hpp"
#include "model/Quest.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace crawl {

class PartyCheckpoint {
private:
    friend class Party;
    int gold = 0;
    std::vector<std::shared_ptr<Item>> inventory;
    std::vector<std::shared_ptr<Character>> members;
    std::vector<std::shared_ptr<Quest>> quests;
    std::unordered_set<std::string> completedQuestIds;
    std::unordered_set<std::string> keyItems;
    Bestiary bestiary;
    DungeonWorld world;
    bool campaignCompleted = false;
    int deepestFloorReached = 1;
    int totalTurns = 0;
    std::uint32_t lastSessionSeed = 0;
    std::uint64_t sessionRngDrawCount = 0;
    bool activeSaveSession = false;
    bool recoveryPending = false;
    std::uint32_t globalRngSeed = 0;
    std::uint64_t globalRngDrawCount = 0;
};

struct SavePreviewInfo {
    bool valid = false;
    bool legacyIncompatible = false;
    int memberCount = 0;
    int gold = 0;
    int deepestFloor = 1;
    int maxLevel = 1;
    bool campaignCompleted = false;
};

class Party {
public:
    static constexpr int SAVE_SCHEMA_VERSION = 5;
    static constexpr int MAX_MEMBERS = 4;

    Party();
    ~Party() = default;

    static void setDefaultSavePath(const std::string& filePath);
    static const std::string& getDefaultSavePath();
    static bool hasRecoverableSave(const std::string& filePath = getDefaultSavePath());
    static SavePreviewInfo inspectSavePreview(const std::string& filePath = getDefaultSavePath());

    bool addMember(std::shared_ptr<Character> member);
    void removeMember(int index);
    [[nodiscard]] const std::vector<std::shared_ptr<Character>>& getMembers() const;
    [[nodiscard]] std::shared_ptr<Character> getMember(int index) const;
    [[nodiscard]] int getMemberCount() const;
    [[nodiscard]] bool hasClassInParty(CharacterClass c) const;

    [[nodiscard]] int getGold() const;
    void addGold(int amount);
    bool spendGold(int amount);

    [[nodiscard]] const std::vector<std::shared_ptr<Item>>& getInventory() const;
    void addItem(std::shared_ptr<Item> item);
    void insertItem(int index, std::shared_ptr<Item> item);
    void removeItem(int index);

    [[nodiscard]] const std::vector<std::shared_ptr<Quest>>& getActiveQuests() const;
    void acceptQuest(std::shared_ptr<Quest> quest);
    void completeQuest(const std::string& questId);
    void abandonQuest(const std::string& questId);
    [[nodiscard]] bool hasQuest(const std::string& questId) const;
    void updateQuestKillProgress(const std::string& monsterId, int count);
    void updateQuestCollectProgress();
    bool markQuestObjectiveComplete(const std::string& questId);
    [[nodiscard]] std::shared_ptr<Quest> getQuest(const std::string& questId) const;

    bool addKeyItem(const std::string& itemId);
    bool removeKeyItem(const std::string& itemId);
    [[nodiscard]] bool hasKeyItem(const std::string& itemId) const;
    [[nodiscard]] const std::unordered_set<std::string>& getKeyItems() const;

    Bestiary& getBestiary() { return m_bestiary; }
    [[nodiscard]] const Bestiary& getBestiary() const { return m_bestiary; }

    DungeonWorld& getWorld();
    [[nodiscard]] const DungeonWorld& getWorld() const;

    [[nodiscard]] int getDeepestFloorReached() const { return m_deepestFloorReached; }
    void markFloorReached(int floorNumber) {
        if (floorNumber > m_deepestFloorReached && floorNumber <= DungeonWorld::FLOOR_COUNT) {
            m_deepestFloorReached = floorNumber;
        }
    }
    [[nodiscard]] int getTotalTurns() const { return m_totalTurns; }
    void incrementTurns(int delta = 1) { m_totalTurns += std::max(0, delta); }

    PersistenceResult saveToFile(const std::string& filePath = getDefaultSavePath());
    PersistenceResult loadFromFile(const std::string& filePath = getDefaultSavePath());

    void resetToDefault();
    PersistenceResult startNewGame(const std::string& filePath = getDefaultSavePath());

    [[nodiscard]] bool isQuestCompleted(const std::string& questId) const;
    [[nodiscard]] const std::unordered_set<std::string>& getCompletedQuestIds() const;
    [[nodiscard]] bool isCampaignCompleted() const;
    void setCampaignCompleted(bool completed);
    [[nodiscard]] std::uint32_t getLastSessionSeed() const;
    void setLastSessionSeed(std::uint32_t seed);
    [[nodiscard]] std::uint64_t getSessionRngDrawCount() const;
    [[nodiscard]] bool hasActiveSaveSession() const;
    [[nodiscard]] bool isRecoveryPending() const;
    void markRecoveryPending();
    [[nodiscard]] bool needsSaveMigration() const { return false; }
    [[nodiscard]] PartyCheckpoint captureCheckpoint() const;
    void restoreCheckpoint(const PartyCheckpoint& checkpoint);

private:
    void validateState() const;

    std::vector<std::shared_ptr<Character>> m_members;
    std::vector<std::shared_ptr<Item>> m_inventory;
    std::vector<std::shared_ptr<Quest>> m_activeQuests;
    std::unordered_set<std::string> m_completedQuestIds;
    std::unordered_set<std::string> m_keyItems;
    Bestiary m_bestiary;
    DungeonWorld m_world;
    int m_gold = 150;
    bool m_campaignCompleted = false;
    int m_deepestFloorReached = 1;
    int m_totalTurns = 0;
    std::uint32_t m_lastSessionSeed = 0;
    std::uint64_t m_sessionRngDrawCount = 0;
    bool m_hasActiveSaveSession = false;
    bool m_recoveryPending = false;
    inline static std::string s_defaultSavePath;
};

} // namespace crawl

#endif // PARTY_HPP
