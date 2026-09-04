// [v0.3.0] Party.cpp 신규 작성
// JSON 기반 세이브 로드 연동, 파일 손상 감지 예외 안전성 복구(try-catch) 및 기본 골드/인벤토리 복원 로직 구현.

#include "model/Party.hpp"
#include "model/ItemFactory.hpp"
#include "core/SessionRng.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace crawl {
namespace {

std::uint32_t deterministicLegacySeed(const nlohmann::json& json) {
    const std::string canonical = "crawlmaster-save-migration-v4|" + json.dump();
    std::uint32_t hash = 2166136261U;
    for (const unsigned char byte : canonical) {
        hash ^= byte;
        hash *= 16777619U;
    }
    return hash == 0U ? 0x9E3779B9U : hash;
}

void requireKeys(const nlohmann::json& object, std::initializer_list<const char*> keys,
                 const char* area) {
    for (const char* key : keys) {
        if (!object.contains(key)) {
            throw std::runtime_error(std::string(area) + " 필수 필드 누락: " + key);
        }
    }
}

bool isCanonicalQuestId(const std::string& id) {
    const auto ids = Quest::getCanonicalIds();
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

} // namespace

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
    if (!member || m_members.size() >= 4) {
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
    if (m_recoveryPending) {
        return {PersistenceStatus::RecoveryPending, filePath,
                "복구가 끝나기 전에는 현재 메모리 상태를 저장할 수 없습니다."};
    }
    const std::uint32_t previousSeed = m_lastSessionSeed;
    const std::uint64_t previousDrawCount = m_sessionRngDrawCount;
    const DungeonWorld previousWorld = m_world;
    const bool previousActiveSession = m_hasActiveSaveSession;
    const int previousSchemaVersion = m_loadedSchemaVersion;
    auto restoreSaveMetadata = [&]() {
        m_lastSessionSeed = previousSeed;
        m_sessionRngDrawCount = previousDrawCount;
        m_world = previousWorld;
        m_hasActiveSaveSession = previousActiveSession;
        m_loadedSchemaVersion = previousSchemaVersion;
    };
    try {
        m_lastSessionSeed = SessionRng::global().seed();
        m_sessionRngDrawCount = SessionRng::global().drawCount();
        nlohmann::json j;
        if (!m_world.isGenerated()) {
            const auto seed = SessionRng::global().seed();
            m_world.generate(seed == 0U ? 0x9E3779B9U : seed);
        }
        validateState();
        j["schemaVersion"] = 4;
        j["gold"] = m_gold;

        // 인벤토리 아이템 ID 리스트 변환
        nlohmann::json invArray = nlohmann::json::array();
        for (const auto& item : m_inventory) {
            if (item) {
                invArray.push_back(item->getId());
            }
        }
        j["inventory"] = invArray;

        std::vector<std::string> sortedKeyItems(m_keyItems.begin(), m_keyItems.end());
        std::sort(sortedKeyItems.begin(), sortedKeyItems.end());
        nlohmann::json keyItems = sortedKeyItems;
        j["keyItems"] = keyItems;

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

        std::vector<std::string> sortedCompletedQuestIds(
            m_completedQuestIds.begin(), m_completedQuestIds.end());
        std::sort(sortedCompletedQuestIds.begin(), sortedCompletedQuestIds.end());
        nlohmann::json completedQuestIds = sortedCompletedQuestIds;
        j["completedQuestIds"] = completedQuestIds;
        j["campaignCompleted"] = m_campaignCompleted;
        j["lastSessionSeed"] = m_lastSessionSeed;
        j["sessionRngDrawCount"] = m_sessionRngDrawCount;
        j["world"] = m_world.toJson();

        auto result = Persistence::atomicWriteText(filePath, j.dump(4));
        if (result.succeeded()) {
            m_hasActiveSaveSession = true;
            m_loadedSchemaVersion = 4;
        }
        if (result.status == PersistenceStatus::CommittedDurabilityUnknown) {
            std::cerr << "[Save Warning] " << result.message << std::endl;
        } else if (result) {
            std::cout << "[Save] 파티 데이터를 세이브 파일(" << filePath << ")에 영속화했습니다." << std::endl;
        } else {
            std::cerr << "[Save Error] " << result.message << std::endl;
            restoreSaveMetadata();
        }
        return result;
    } catch (const std::exception& e) {
        restoreSaveMetadata();
        std::cerr << "[Save Error] 세이브 중 예외가 발생했습니다: " << e.what() << std::endl;
        return {PersistenceStatus::IoError, filePath, e.what()};
    }
}

PersistenceResult Party::loadFromFile(const std::string& filePath) {
    auto loadCandidate = [this](const std::filesystem::path& candidate,
                                PersistenceStatus successStatus) -> PersistenceResult {
        std::error_code error;
        if (std::filesystem::is_symlink(std::filesystem::symlink_status(candidate, error))) {
            return {PersistenceStatus::IoError, candidate, "심볼릭 링크 세이브는 읽지 않습니다."};
        }
        error.clear();
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
            if (!j.contains("schemaVersion") &&
                (j.contains("world") || j.contains("keyItems") || j.contains("completedQuestIds") ||
                 j.contains("sessionRngDrawCount"))) {
                throw std::runtime_error("canonical save의 schemaVersion이 누락됐습니다.");
            }

            const int schemaVersion = j.value("schemaVersion", 1);
            if (schemaVersion < 1 || schemaVersion > 4) {
                return {PersistenceStatus::UnsupportedVersion, candidate,
                        "지원하지 않는 세이브 스키마입니다: " + std::to_string(schemaVersion)};
            }
            if (schemaVersion == 4) {
                requireKeys(j, {"schemaVersion", "gold", "inventory", "keyItems", "members",
                                "activeQuests", "completedQuestIds", "campaignCompleted",
                                "lastSessionSeed", "sessionRngDrawCount", "world"}, "v4 save");
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

            std::unordered_set<std::string> keyItems;
            if (schemaVersion >= 4) {
                const auto& keyItemsJson = j.at("keyItems");
                if (!keyItemsJson.is_array() || keyItemsJson.size() > 32) {
                    throw std::runtime_error("keyItems 형식 또는 크기가 잘못됐습니다.");
                }
                for (const auto& keyItemJson : keyItemsJson) {
                    const std::string itemId = keyItemJson.get<std::string>();
                    if (itemId != "key_moon_seal" || !keyItems.insert(itemId).second) {
                        throw std::runtime_error("알 수 없거나 중복된 중요품입니다.");
                    }
                }
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
            if (schemaVersion >= 4 || j.contains(activeQuestKey)) {
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
            if (schemaVersion >= 4 || (schemaVersion >= 2 && j.contains("completedQuestIds"))) {
                const auto& completedJson = j.at("completedQuestIds");
                if (!completedJson.is_array() || completedJson.size() > 100) {
                    throw std::runtime_error("completedQuestIds 형식 또는 크기가 잘못됐습니다.");
                }
                for (const auto& questId : completedJson) {
                    const std::string id = questId.get<std::string>();
                    if (!isCanonicalQuestId(id) || !completedQuestIds.insert(id).second) {
                        throw std::runtime_error("completed quest id가 unknown 또는 duplicate입니다.");
                    }
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

            const bool campaignCompleted = schemaVersion >= 4 ? j.at("campaignCompleted").get<bool>() :
                (schemaVersion >= 2 ? j.value("campaignCompleted", false) : false);
            std::uint32_t lastSessionSeed = schemaVersion >= 4
                ? j.at("lastSessionSeed").get<std::uint32_t>()
                : (schemaVersion >= 2 ? j.value("lastSessionSeed", std::uint32_t{0}) : 0);
            const std::uint64_t sessionRngDrawCount = schemaVersion >= 2
                ? (schemaVersion >= 4 ? j.at("sessionRngDrawCount").get<std::uint64_t>()
                                      : j.value("sessionRngDrawCount", std::uint64_t{0})) : 0;
            if (sessionRngDrawCount > 10'000'000U) {
                throw std::runtime_error("sessionRngDrawCount 범위를 벗어났습니다.");
            }

            if (lastSessionSeed == 0U) {
                if (schemaVersion >= 4) throw std::runtime_error("v4 session seed는 0일 수 없습니다.");
                lastSessionSeed = deterministicLegacySeed(j);
            }
            DungeonWorld world;
            if (schemaVersion >= 4) {
                world = DungeonWorld::fromJson(j.at("world"));
                if (world.getSeed() != lastSessionSeed) {
                    throw std::runtime_error("world seed와 session RNG seed가 일치하지 않습니다.");
                }
            } else {
                world.generate(lastSessionSeed);
            }

            for (const auto& object : world.getObjects()) {
                const auto active = std::find_if(activeQuests.begin(), activeQuests.end(),
                    [&](const auto& quest) { return quest && quest->getId() == object.questId; });
                const bool isActive = active != activeQuests.end();
                const bool isCompleted = completedQuestIds.contains(object.questId);
                if (object.state == WorldObjectState::RESOLVED && !isActive && !isCompleted) {
                    throw std::runtime_error("해결된 월드 목표에 대응하는 퀘스트 진행이 없습니다.");
                }
                if (isActive) {
                    const bool ready = (*active)->isReadyToReport();
                    if (ready != (object.state == WorldObjectState::RESOLVED)) {
                        throw std::runtime_error("월드 목표와 퀘스트 보고 상태가 일치하지 않습니다.");
                    }
                    if ((*active)->getType() == QuestType::RETRIEVE_KEY_ITEM && ready &&
                        !keyItems.contains((*active)->getTargetId())) {
                        throw std::runtime_error("회수 완료 퀘스트의 중요품이 없습니다.");
                    }
                    if ((*active)->getType() == QuestType::RETRIEVE_KEY_ITEM && !ready &&
                        keyItems.contains((*active)->getTargetId())) {
                        throw std::runtime_error("미완료 회수 퀘스트가 중요품을 이미 보유합니다.");
                    }
                }
                if (isCompleted && object.state != WorldObjectState::RESOLVED) {
                    throw std::runtime_error("완료 퀘스트의 월드 목표가 해결되지 않았습니다.");
                }
            }
            for (const auto& keyItem : keyItems) {
                const auto quest = std::find_if(activeQuests.begin(), activeQuests.end(),
                    [&](const auto& candidate) {
                        return candidate && candidate->getType() == QuestType::RETRIEVE_KEY_ITEM &&
                               candidate->getTargetId() == keyItem && candidate->isReadyToReport();
                    });
                if (quest == activeQuests.end() || completedQuestIds.contains((*quest)->getId())) {
                    throw std::runtime_error("중요품에 대응하는 보고 대기 퀘스트가 없습니다.");
                }
            }
            if (completedQuestIds.contains("qst_recover_moon_seal") &&
                keyItems.contains("key_moon_seal")) {
                throw std::runtime_error("완료된 회수 퀘스트의 중요품이 남아 있습니다.");
            }

            m_gold = gold;
            m_inventory = std::move(inventory);
            m_members = std::move(members);
            m_activeQuests = std::move(activeQuests);
            m_completedQuestIds = std::move(completedQuestIds);
            m_keyItems = std::move(keyItems);
            m_world = std::move(world);
            m_campaignCompleted = campaignCompleted;
            m_lastSessionSeed = lastSessionSeed;
            m_sessionRngDrawCount = sessionRngDrawCount;
            m_hasActiveSaveSession = true;
            m_recoveryPending = false;
            m_loadedSchemaVersion = schemaVersion;
            SessionRng::global() = SessionRng(lastSessionSeed, sessionRngDrawCount);

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

    auto quarantineCorruptCandidate = [](const std::filesystem::path& candidate,
                                         const PersistenceResult& corruptResult) {
        std::filesystem::path quarantinePath;
        const auto quarantineResult = Persistence::quarantine(candidate, quarantinePath);
        if (quarantineResult.status == PersistenceStatus::IoError) return quarantineResult;
        std::string message = corruptResult.message;
        if (quarantineResult.status == PersistenceStatus::CommittedDurabilityUnknown) {
            message += " 손상 파일은 격리됐지만 디렉터리 동기화를 확인하지 못했습니다.";
        }
        return PersistenceResult{PersistenceStatus::Corrupt, quarantinePath, std::move(message)};
    };

    if (primaryResult.status == PersistenceStatus::NotFound && std::filesystem::exists(backup)) {
        auto result = recoverBackup();
        if (result.status == PersistenceStatus::Corrupt) {
            return quarantineCorruptCandidate(backup, result);
        }
        return result;
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

    if (backupResult.status == PersistenceStatus::Corrupt) {
        const auto backupQuarantineResult = quarantineCorruptCandidate(backup, backupResult);
        if (backupQuarantineResult.status == PersistenceStatus::IoError) {
            return backupQuarantineResult;
        }
    }

    std::cerr << "[Load Warning] 손상 세이브를 격리했습니다: " << corruptionReason << std::endl;
    return {PersistenceStatus::Corrupt, quarantinePath, corruptionReason};
}

void Party::resetToDefault() {
    m_members.clear();
    m_inventory.clear();
    m_activeQuests.clear();
    m_completedQuestIds.clear();
    m_keyItems.clear();
    m_world = DungeonWorld{};
    m_campaignCompleted = false;
    m_lastSessionSeed = 0;
    m_sessionRngDrawCount = 0;
    m_hasActiveSaveSession = false;
    m_loadedSchemaVersion = 4;
    // 기본 치유물약 2개와 마나 물약 1개 지급 (spec.md 정책)
    m_inventory.push_back(ItemFactory::createItem("pot_heal"));
    m_inventory.push_back(ItemFactory::createItem("pot_heal"));
    m_inventory.push_back(ItemFactory::createItem("pot_mana"));
    m_gold = 100;
}

PersistenceResult Party::startNewGame(const std::string& filePath) {
    const PartyCheckpoint checkpoint = captureCheckpoint();
    resetToDefault();
    m_recoveryPending = false;
    m_lastSessionSeed = SessionRng::global().seed();
    m_world.generate(m_lastSessionSeed == 0U ? 0x9E3779B9U : m_lastSessionSeed);
    const auto result = saveToFile(filePath);
    if (!result.succeeded()) restoreCheckpoint(checkpoint);
    return result;
}

const std::vector<std::shared_ptr<Quest>>& Party::getActiveQuests() const {
    return m_activeQuests;
}

void Party::acceptQuest(std::shared_ptr<Quest> quest) {
    if (!quest || hasQuest(quest->getId()) || isQuestCompleted(quest->getId())) return;
    if (!quest->matchesCanonicalDefinition()) return;
    m_activeQuests.push_back(std::move(quest));
}

void Party::completeQuest(const std::string& questId) {
    if (isQuestCompleted(questId)) return;
    auto it = std::find_if(m_activeQuests.begin(), m_activeQuests.end(),
        [&](const auto& q) { return q->getId() == questId; });

    if (it != m_activeQuests.end()) {
        auto quest = *it;
        if (quest->checkCompletion()) {
            if (quest->getTargetFloor() > 0) {
                const auto object = std::find_if(m_world.getObjects().begin(), m_world.getObjects().end(),
                    [&](const auto& candidate) { return candidate.questId == questId; });
                if (object == m_world.getObjects().end() || object->state != WorldObjectState::RESOLVED) return;
            }
            if (quest->getType() == QuestType::RETRIEVE_KEY_ITEM && !hasKeyItem(quest->getTargetId())) {
                return;
            }
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
            if (quest->getType() == QuestType::RETRIEVE_KEY_ITEM) {
                removeKeyItem(quest->getTargetId());
            }

            // 활성 퀘스트 리스트에서 완료 퀘스트 제거
            for (const auto& rewardItemId : quest->getRewardItemIds()) {
                auto rewardItem = ItemFactory::createItem(rewardItemId);
                if (rewardItem) m_inventory.push_back(std::move(rewardItem));
            }
            quest->setCompleted(true);
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
        // 현장에서 해결된 목적형 퀘스트를 버리면 영속 월드와 중요품 상태가 고아가 된다.
        if ((*it)->getTargetFloor() > 0 && (*it)->isReadyToReport()) return;
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

bool Party::markQuestObjectiveComplete(const std::string& questId) {
    auto quest = getQuest(questId);
    return quest && quest->markObjectiveComplete();
}

std::shared_ptr<Quest> Party::getQuest(const std::string& questId) const {
    const auto it = std::find_if(m_activeQuests.begin(), m_activeQuests.end(),
        [&](const auto& quest) { return quest && quest->getId() == questId; });
    return it == m_activeQuests.end() ? nullptr : *it;
}

bool Party::addKeyItem(const std::string& itemId) {
    if (itemId != "key_moon_seal") return false;
    return m_keyItems.insert(itemId).second;
}

bool Party::removeKeyItem(const std::string& itemId) {
    return m_keyItems.erase(itemId) > 0;
}

bool Party::hasKeyItem(const std::string& itemId) const {
    return m_keyItems.contains(itemId);
}

const std::unordered_set<std::string>& Party::getKeyItems() const { return m_keyItems; }
DungeonWorld& Party::getWorld() { return m_world; }
const DungeonWorld& Party::getWorld() const { return m_world; }

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

bool Party::hasActiveSaveSession() const { return m_hasActiveSaveSession; }
bool Party::isRecoveryPending() const { return m_recoveryPending; }
void Party::markRecoveryPending() { m_recoveryPending = true; }
bool Party::needsSaveMigration() const { return m_hasActiveSaveSession && m_loadedSchemaVersion < 4; }

PartyCheckpoint Party::captureCheckpoint() const {
    PartyCheckpoint checkpoint;
    checkpoint.gold = m_gold;
    for (const auto& item : m_inventory) {
        if (item) checkpoint.inventoryIds.push_back(item->getId());
    }
    for (const auto& member : m_members) {
        if (member) checkpoint.members.push_back(std::make_shared<Character>(*member));
    }
    for (const auto& quest : m_activeQuests) {
        if (quest) checkpoint.quests.push_back(std::make_shared<Quest>(*quest));
    }
    checkpoint.completedQuestIds = m_completedQuestIds;
    checkpoint.keyItems = m_keyItems;
    checkpoint.world = m_world;
    checkpoint.campaignCompleted = m_campaignCompleted;
    checkpoint.lastSessionSeed = m_lastSessionSeed;
    checkpoint.sessionRngDrawCount = m_sessionRngDrawCount;
    checkpoint.activeSaveSession = m_hasActiveSaveSession;
    checkpoint.recoveryPending = m_recoveryPending;
    checkpoint.loadedSchemaVersion = m_loadedSchemaVersion;
    checkpoint.globalRngSeed = SessionRng::global().seed();
    checkpoint.globalRngDrawCount = SessionRng::global().drawCount();
    return checkpoint;
}

void Party::restoreCheckpoint(const PartyCheckpoint& checkpoint) {
    std::vector<std::shared_ptr<Item>> inventory;
    for (const auto& itemId : checkpoint.inventoryIds) {
        auto item = ItemFactory::createItem(itemId);
        if (!item) throw std::runtime_error("checkpoint item 복원에 실패했습니다.");
        inventory.push_back(std::move(item));
    }
    std::vector<std::shared_ptr<Character>> members;
    for (const auto& member : checkpoint.members) members.push_back(std::make_shared<Character>(*member));
    std::vector<std::shared_ptr<Quest>> quests;
    for (const auto& quest : checkpoint.quests) quests.push_back(std::make_shared<Quest>(*quest));

    m_gold = checkpoint.gold;
    m_inventory = std::move(inventory);
    m_members = std::move(members);
    m_activeQuests = std::move(quests);
    m_completedQuestIds = checkpoint.completedQuestIds;
    m_keyItems = checkpoint.keyItems;
    m_world = checkpoint.world;
    m_campaignCompleted = checkpoint.campaignCompleted;
    m_lastSessionSeed = checkpoint.lastSessionSeed;
    m_sessionRngDrawCount = checkpoint.sessionRngDrawCount;
    m_hasActiveSaveSession = checkpoint.activeSaveSession;
    m_recoveryPending = checkpoint.recoveryPending;
    m_loadedSchemaVersion = checkpoint.loadedSchemaVersion;
    SessionRng::global() = SessionRng(checkpoint.globalRngSeed, checkpoint.globalRngDrawCount);
}

void Party::validateState() const {
    if (m_members.size() > 4 ||
        std::any_of(m_members.begin(), m_members.end(), [](const auto& member) { return !member; }) ||
        std::any_of(m_inventory.begin(), m_inventory.end(), [](const auto& item) { return !item; })) {
        throw std::runtime_error("Party member/inventory invariant가 잘못됐습니다.");
    }
    m_world.validate();
    if (m_world.getSeed() != m_lastSessionSeed) {
        throw std::runtime_error("world seed와 session RNG seed가 일치하지 않습니다.");
    }
    std::unordered_set<std::string> activeIds;
    for (const auto& quest : m_activeQuests) {
        if (!quest || !quest->matchesCanonicalDefinition() ||
            !activeIds.insert(quest->getId()).second ||
            m_completedQuestIds.contains(quest->getId())) {
            throw std::runtime_error("active quest invariant가 잘못됐습니다.");
        }
    }
    for (const auto& id : m_completedQuestIds) {
        if (!isCanonicalQuestId(id)) throw std::runtime_error("completed quest id가 canonical이 아닙니다.");
    }
    for (const auto& object : m_world.getObjects()) {
        const auto active = getQuest(object.questId);
        const bool completed = m_completedQuestIds.contains(object.questId);
        if (object.state == WorldObjectState::RESOLVED) {
            if ((!active || !active->isReadyToReport()) && !completed) {
                throw std::runtime_error("resolved object에 대응하는 quest 상태가 없습니다.");
            }
        } else if ((active && active->isReadyToReport()) || completed) {
            throw std::runtime_error("quest 완료 상태와 world object가 일치하지 않습니다.");
        }
    }
    const auto retrieve = getQuest("qst_recover_moon_seal");
    const bool hasMoonSeal = m_keyItems.contains("key_moon_seal");
    if (hasMoonSeal != (retrieve && retrieve->isReadyToReport()) ||
        (m_completedQuestIds.contains("qst_recover_moon_seal") && hasMoonSeal)) {
        throw std::runtime_error("중요품과 회수 quest 상태가 일치하지 않습니다.");
    }
    if (m_keyItems.size() > 1 ||
        std::any_of(m_keyItems.begin(), m_keyItems.end(), [](const auto& id) { return id != "key_moon_seal"; })) {
        throw std::runtime_error("알 수 없는 중요품이 있습니다.");
    }
}

} // namespace crawl
