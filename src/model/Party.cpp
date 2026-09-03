// [v0.3.0] Party.cpp 신규 작성
// JSON 기반 세이브 로드 연동, 파일 손상 감지 예외 안전성 복구(try-catch) 및 기본 골드/인벤토리 복원 로직 구현.

#include "model/Party.hpp"
#include "model/ItemFactory.hpp"
#include "core/SessionRng.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace crawl {

Party::Party() : m_gold(100) {
    resetToDefault();
}

void Party::setDefaultSavePath(const std::string& filePath) {
    s_defaultSavePath = filePath;
}

const std::string& Party::getDefaultSavePath() {
    if (s_defaultSavePath.empty()) {
        s_defaultSavePath = Persistence::defaultSavePath().string();
    }
    return s_defaultSavePath;
}

bool Party::hasRecoverableSave(const std::string& filePath) {
    return std::filesystem::exists(filePath) || std::filesystem::exists(filePath + ".bak");
}

bool Party::addMember(std::shared_ptr<Character> member) {
    if (m_members.size() >= 4) {
        std::cerr << "[Party] 파티원이 꽉 찼습니다 (최대 4인)." << std::endl;
        return false;
    }
    m_members.push_back(member);
    return true;
}

void Party::removeMember(int index) {
    if (index >= 0 && index < static_cast<int>(m_members.size())) {
        m_members.erase(m_members.begin() + index);
    }
}

const std::vector<std::shared_ptr<Character>>& Party::getMembers() const {
    return m_members;
}

std::shared_ptr<Character> Party::getMember(int index) const {
    if (index >= 0 && index < static_cast<int>(m_members.size())) {
        return m_members[index];
    }
    return nullptr;
}

int Party::getMemberCount() const {
    return static_cast<int>(m_members.size());
}

int Party::getGold() const {
    return m_gold;
}

void Party::addGold(int amount) {
    m_gold += amount;
}

bool Party::spendGold(int amount) {
    if (m_gold >= amount) {
        m_gold -= amount;
        return true;
    }
    return false;
}

const std::vector<std::shared_ptr<Item>>& Party::getInventory() const {
    return m_inventory;
}

void Party::addItem(std::shared_ptr<Item> item) {
    if (item) {
        m_inventory.push_back(item);
    }
}

void Party::insertItem(int index, std::shared_ptr<Item> item) {
    if (!item) return;
    const int boundedIndex = std::clamp(index, 0, static_cast<int>(m_inventory.size()));
    m_inventory.insert(m_inventory.begin() + boundedIndex, std::move(item));
}

void Party::removeItem(int index) {
    if (index >= 0 && index < static_cast<int>(m_inventory.size())) {
        m_inventory.erase(m_inventory.begin() + index);
    }
}

PersistenceResult Party::saveToFile(const std::string& filePath) {
    try {
        m_lastSessionSeed = SessionRng::global().seed();
        m_sessionRngDrawCount = SessionRng::global().drawCount();
        nlohmann::json j;
        j["schemaVersion"] = 2;
        j["gold"] = m_gold;

        // 인벤토리 아이템 ID 리스트 변환
        nlohmann::json invArray = nlohmann::json::array();
        for (const auto& item : m_inventory) {
            if (item) {
                invArray.push_back(item->getId());
            }
        }
        j["inventory"] = invArray;

        // 파티원 캐릭터 정보 직렬화
        nlohmann::json membersArray = nlohmann::json::array();
        for (const auto& member : m_members) {
            if (member) {
                membersArray.push_back(member->toJson());
            }
        }
        j["members"] = membersArray;

        // 활성 수락 퀘스트 직렬화
        nlohmann::json questsArray = nlohmann::json::array();
        for (const auto& quest : m_activeQuests) {
            if (quest) {
                questsArray.push_back(quest->toJson());
            }
        }
        j["activeQuests"] = questsArray;

        nlohmann::json completedQuestIds = nlohmann::json::array();
        for (const auto& questId : m_completedQuestIds) {
            completedQuestIds.push_back(questId);
        }
        j["completedQuestIds"] = completedQuestIds;
        j["campaignCompleted"] = m_campaignCompleted;
        j["lastSessionSeed"] = m_lastSessionSeed;
        j["sessionRngDrawCount"] = m_sessionRngDrawCount;

        auto result = Persistence::atomicWriteText(filePath, j.dump(4));
        if (result.status == PersistenceStatus::CommittedDurabilityUnknown) {
            std::cerr << "[Save Warning] " << result.message << std::endl;
        } else if (result) {
            std::cout << "[Save] 파티 데이터를 세이브 파일(" << filePath << ")에 영속화했습니다." << std::endl;
        } else {
            std::cerr << "[Save Error] " << result.message << std::endl;
        }
        return result;
    } catch (const std::exception& e) {
        std::cerr << "[Save Error] 세이브 중 예외가 발생했습니다: " << e.what() << std::endl;
        return {PersistenceStatus::IoError, filePath, e.what()};
    }
}

PersistenceResult Party::loadFromFile(const std::string& filePath) {
    auto loadCandidate = [this](const std::filesystem::path& candidate,
                                PersistenceStatus successStatus) -> PersistenceResult {
        std::error_code error;
        if (!std::filesystem::exists(candidate, error)) {
            return {PersistenceStatus::NotFound, candidate, "세이브 파일이 없습니다."};
        }

        std::ifstream file(candidate, std::ios::binary);
        if (!file.is_open()) {
            return {PersistenceStatus::IoError, candidate, "세이브 파일을 읽을 수 없습니다."};
        }

        const auto fileSize = std::filesystem::file_size(candidate, error);
        if (error) return {PersistenceStatus::IoError, candidate, error.message()};
        if (fileSize > 1024U * 1024U) {
            return {PersistenceStatus::Corrupt, candidate, "save 파일이 1 MiB 제한을 초과했습니다."};
        }

        try {
            nlohmann::json j;
            file >> j;
            if (!j.is_object()) throw std::runtime_error("세이브 루트는 객체여야 합니다.");

            const int schemaVersion = j.value("schemaVersion", 1);
            if (schemaVersion < 1 || schemaVersion > 2) {
                return {PersistenceStatus::UnsupportedVersion, candidate,
                        "지원하지 않는 세이브 스키마입니다: " + std::to_string(schemaVersion)};
            }

            const int gold = j.at("gold").get<int>();
            if (gold < 0 || gold > 1'000'000'000) {
                throw std::runtime_error("gold 범위를 벗어났습니다.");
            }

            std::vector<std::shared_ptr<Item>> inventory;
            const auto& inventoryJson = j.at("inventory");
            if (!inventoryJson.is_array() || inventoryJson.size() > 1'000) {
                throw std::runtime_error("inventory 형식 또는 크기가 잘못됐습니다.");
            }
            for (const auto& itemIdJson : inventoryJson) {
                const std::string itemId = itemIdJson.get<std::string>();
                auto item = ItemFactory::createItem(itemId);
                if (!item) throw std::runtime_error("알 수 없는 item id: " + itemId);
                inventory.push_back(std::move(item));
            }

            std::vector<std::shared_ptr<Character>> members;
            const auto& membersJson = j.at("members");
            if (!membersJson.is_array() || membersJson.size() > 4) {
                throw std::runtime_error("members 형식 또는 최대 4인 제한을 위반했습니다.");
            }
            for (const auto& characterJson : membersJson) {
                auto character = Character::fromJson(characterJson, schemaVersion);
                if (!character) throw std::runtime_error("캐릭터 복원에 실패했습니다.");
                members.push_back(std::move(character));
            }

            const char* activeQuestKey = schemaVersion == 1 ? "active_quests" : "activeQuests";
            std::vector<std::shared_ptr<Quest>> activeQuests;
            if (j.contains(activeQuestKey)) {
                const auto& questsJson = j.at(activeQuestKey);
                if (!questsJson.is_array() || questsJson.size() > 100) {
                    throw std::runtime_error("active quest 형식 또는 크기가 잘못됐습니다.");
                }
                for (const auto& questJson : questsJson) {
                    auto quest = Quest::fromJson(questJson, schemaVersion);
                    if (!quest) throw std::runtime_error("퀘스트 복원에 실패했습니다.");
                    activeQuests.push_back(std::move(quest));
                }
            }

            std::unordered_set<std::string> completedQuestIds;
            if (schemaVersion == 2 && j.contains("completedQuestIds")) {
                const auto& completedJson = j.at("completedQuestIds");
                if (!completedJson.is_array() || completedJson.size() > 100) {
                    throw std::runtime_error("completedQuestIds 형식 또는 크기가 잘못됐습니다.");
                }
                for (const auto& questId : completedJson) {
                    const std::string id = questId.get<std::string>();
                    if (id.empty() || id.size() > 128) {
                        throw std::runtime_error("completed quest id 길이가 잘못됐습니다.");
                    }
                    completedQuestIds.insert(id);
                }
            }

            std::unordered_set<std::string> activeQuestIds;
            for (const auto& quest : activeQuests) {
                if (!quest || !activeQuestIds.insert(quest->getId()).second) {
                    throw std::runtime_error("중복 active quest id가 있습니다.");
                }
                if (completedQuestIds.contains(quest->getId())) {
                    throw std::runtime_error("active/completed quest가 겹칩니다.");
                }
            }

            const bool campaignCompleted = schemaVersion == 2 ? j.value("campaignCompleted", false) : false;
            const std::uint32_t lastSessionSeed = schemaVersion == 2
                ? j.value("lastSessionSeed", std::uint32_t{0}) : 0;
            const std::uint64_t sessionRngDrawCount = schemaVersion == 2
                ? j.value("sessionRngDrawCount", std::uint64_t{0}) : 0;
            if (sessionRngDrawCount > 10'000'000U) {
                throw std::runtime_error("sessionRngDrawCount 범위를 벗어났습니다.");
            }

            m_gold = gold;
            m_inventory = std::move(inventory);
            m_members = std::move(members);
            m_activeQuests = std::move(activeQuests);
            m_completedQuestIds = std::move(completedQuestIds);
            m_campaignCompleted = campaignCompleted;
            m_lastSessionSeed = lastSessionSeed;
            m_sessionRngDrawCount = sessionRngDrawCount;

            std::cout << "[Load] 세이브 파일(" << candidate.string() << ")로부터 파티 데이터를 로드했습니다." << std::endl;
            return {successStatus, candidate, {}};
        } catch (const std::exception& exception) {
            return {PersistenceStatus::Corrupt, candidate, exception.what()};
        }
    };

    const std::filesystem::path primary(filePath);
    auto primaryResult = loadCandidate(primary, PersistenceStatus::Loaded);
    const std::filesystem::path backup = filePath + ".bak";
    auto recoverBackup = [&]() -> PersistenceResult {
        auto backupResult = loadCandidate(backup, PersistenceStatus::RecoveredFromBackup);
        if (!backupResult) return backupResult;

        std::ifstream backupFile(backup, std::ios::binary);
        const std::string backupBytes{std::istreambuf_iterator<char>(backupFile),
                                      std::istreambuf_iterator<char>()};
        const auto restoreResult = Persistence::atomicWriteText(primary, backupBytes);
        backupResult.message = restoreResult
            ? "백업을 로드하고 primary save를 복원했습니다."
            : "백업은 로드했지만 primary save 복원에 실패했습니다: " + restoreResult.message;
        return backupResult;
    };

    if (primaryResult.status == PersistenceStatus::NotFound && std::filesystem::exists(backup)) {
        return recoverBackup();
    }
    if (primaryResult.status != PersistenceStatus::Corrupt) {
        return primaryResult;
    }

    const std::string corruptionReason = primaryResult.message;
    std::filesystem::path quarantinePath;
    const auto quarantineResult = Persistence::quarantine(primary, quarantinePath);
    if (quarantineResult.status == PersistenceStatus::IoError) {
        return quarantineResult;
    }

    auto backupResult = recoverBackup();
    if (backupResult) {
        backupResult.message = "손상 원본을 " + quarantinePath.string() + "에 격리했습니다. " + backupResult.message;
        return backupResult;
    }

    std::cerr << "[Load Warning] 손상 세이브를 격리했습니다: " << corruptionReason << std::endl;
    return {PersistenceStatus::Corrupt, quarantinePath, corruptionReason};
}

void Party::resetToDefault() {
    m_members.clear();
    m_inventory.clear();
    m_activeQuests.clear();
    m_completedQuestIds.clear();
    m_campaignCompleted = false;
    m_lastSessionSeed = 0;
    m_sessionRngDrawCount = 0;
    // 기본 치유물약 2개와 마나 물약 1개 지급 (spec.md 정책)
    m_inventory.push_back(ItemFactory::createItem("pot_heal"));
    m_inventory.push_back(ItemFactory::createItem("pot_heal"));
    m_inventory.push_back(ItemFactory::createItem("pot_mana"));
    m_gold = 100;
}

PersistenceResult Party::startNewGame(const std::string& filePath) {
    resetToDefault();
    m_lastSessionSeed = SessionRng::global().seed();
    return saveToFile(filePath);
}

const std::vector<std::shared_ptr<Quest>>& Party::getActiveQuests() const {
    return m_activeQuests;
}

void Party::acceptQuest(std::shared_ptr<Quest> quest) {
    if (quest && !hasQuest(quest->getId()) && !isQuestCompleted(quest->getId())) {
        m_activeQuests.push_back(quest);
    }
}

void Party::completeQuest(const std::string& questId) {
    if (isQuestCompleted(questId)) return;
    auto it = std::find_if(m_activeQuests.begin(), m_activeQuests.end(),
        [&](const auto& q) { return q->getId() == questId; });

    if (it != m_activeQuests.end()) {
        auto quest = *it;
        if (quest->checkCompletion()) {
            // 보상 지급
            addGold(quest->getGoldReward());
            
            // 살아있는 파티원 멤버들에게 균등 분배
            int aliveCount = 0;
            for (const auto& member : m_members) {
                if (member && !member->isDead()) aliveCount++;
            }

            if (aliveCount > 0) {
                int xpShare = quest->getXpReward() / aliveCount;
                for (auto& member : m_members) {
                    if (member && !member->isDead()) {
                        member->addXp(xpShare);
                    }
                }
            }

            // COLLECT 타입의 수집형 퀘스트인 경우 인벤토리에서 실제 아이템 삭제
            if (quest->getType() == QuestType::COLLECT) {
                std::string targetId = quest->getTargetId();
                int targetCount = quest->getTargetCount();
                int removed = 0;

                while (removed < targetCount) {
                    auto invIt = std::find_if(m_inventory.begin(), m_inventory.end(),
                        [&](const auto& item) { return item && item->getId() == targetId; });

                    if (invIt != m_inventory.end()) {
                        m_inventory.erase(invIt);
                        removed++;
                    } else {
                        break;
                    }
                }
            }

            // 활성 퀘스트 리스트에서 완료 퀘스트 제거
            for (const auto& rewardItemId : quest->getRewardItemIds()) {
                auto rewardItem = ItemFactory::createItem(rewardItemId);
                if (rewardItem) m_inventory.push_back(std::move(rewardItem));
            }
            m_completedQuestIds.insert(questId);
            m_activeQuests.erase(it);
            std::cout << "[Quest] 퀘스트 " << questId << "를 완료하고 보상을 정산했습니다." << std::endl;
        }
    }
}

void Party::abandonQuest(const std::string& questId) {
    auto it = std::find_if(m_activeQuests.begin(), m_activeQuests.end(),
        [&](const auto& q) { return q->getId() == questId; });

    if (it != m_activeQuests.end()) {
        m_activeQuests.erase(it);
    }
}

bool Party::hasQuest(const std::string& questId) const {
    return std::any_of(m_activeQuests.begin(), m_activeQuests.end(),
        [&](const auto& q) { return q && q->getId() == questId; });
}

void Party::updateQuestKillProgress(const std::string& monsterId, int count) {
    for (auto& quest : m_activeQuests) {
        if (quest) {
            quest->updateProgress(monsterId, count);
        }
    }
}

void Party::updateQuestCollectProgress() {
    for (auto& quest : m_activeQuests) {
        if (quest && quest->getType() == QuestType::COLLECT) {
            std::string targetId = quest->getTargetId();
            // 가방 내 해당 아이템 개수 카운트
            int count = 0;
            for (const auto& item : m_inventory) {
                if (item && item->getId() == targetId) {
                    count++;
                }
            }
            quest->setCurrentCount(count);
        }
    }
}

bool Party::isQuestCompleted(const std::string& questId) const {
    return m_completedQuestIds.contains(questId);
}

const std::unordered_set<std::string>& Party::getCompletedQuestIds() const {
    return m_completedQuestIds;
}

bool Party::isCampaignCompleted() const {
    return m_campaignCompleted;
}

void Party::setCampaignCompleted(bool completed) {
    m_campaignCompleted = completed;
}

std::uint32_t Party::getLastSessionSeed() const {
    return m_lastSessionSeed;
}

void Party::setLastSessionSeed(std::uint32_t seed) {
    m_lastSessionSeed = seed;
}

std::uint64_t Party::getSessionRngDrawCount() const {
    return m_sessionRngDrawCount;
}

} // namespace crawl
